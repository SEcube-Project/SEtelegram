/**
  ******************************************************************************
  * File Name          : AccessWindow.cpp
  * Description        :
  ******************************************************************************
  *
  * Copyright © 2016-present Blu5 Group <https://www.blu5group.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 3 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, see <https://www.gnu.org/licenses/>.
  *
  ******************************************************************************
  */

#include "AccessWindow.h"
#include "ui_AccessWindow.h"
#include "ChatWindow.h"
#include "Dialogs/ErrorDialog.h"
#include "Td.h"
#include <QStyle>
#include <QDesktopWidget>
#include <iostream>
#include <cerrno>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "global.h"
#include "Dialogs/TelegramAccessDialog.h"
//#include <csignal>
#ifdef _WIN32
#include <Windows.h>
#else
//#include <unistd.h>
#endif

AccessWindow::AccessWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::AccessWindow) {
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,this->size(), qApp->desktop()->availableGeometry()));
    /* SOCKET for communication with the SEcube SDK Service Provider (SEcubeServer.exe) */
    SEcube_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (SEcube_socket < 0) {
        std::string errstring = "GUI: Error creating socket: ";
        errstring.append(strerror(errno));
        printerr(errstring);
        return;
    }
    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    int cnt = 0;
    while (true) {
        if (::connect(SEcube_socket, (struct sockaddr *)&server, sizeof(server)) == 0) {
            logmessage("GUI: Connected");
            break;
        }
        std::string errstring = "GUI: Connection with SEcube server failed: ";
        errstring.append(strerror(errno));
        printerr(errstring);
        if(cnt > 10){
            QMessageBox errormessage;
            errormessage.setIcon(QMessageBox::Critical);
            errormessage.setText("Error, the internal thread used to communicate with the SEcube device is not running.");
            errormessage.setInformativeText("Press ok to try again in 2 seconds, press cancel to abort.");
            errormessage.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
            errormessage.setDefaultButton(QMessageBox::Ok);
            if(errormessage.exec() == QMessageBox::Ok){
                sleep(2);
            } else {
                ::close(SEcube_socket);
                return;
            }
        }
        cnt++;
    }
    /* SOCKET Connected */
    ui->loadingLabel->hide();
    ui->accessButton->setEnabled(false);
    connect(ui->pinEdit, &QLineEdit::textChanged, [this](){ui->accessButton->setEnabled(!ui->pinEdit->text().isEmpty());});
    connect(ui->accessButton, &QPushButton::clicked, this, &AccessWindow::accessButtonClicked);
    socket_ok = true;
}

void AccessWindow::closeEvent(QCloseEvent *event) {
    if (!setup_ok) {
        logmessage("Joining secube thread");
        send(SEcube_socket, "quit", 4, 0);
        ::close(SEcube_socket);
        secube_thread.join();
        logmessage("Secube thread joined");
    }
}

AccessWindow::~AccessWindow() {
    delete ui;
    if (setup_ok) {
        delete cw;
    }
}

void AccessWindow::accessButtonClicked() {
    ui->accessButton->setEnabled(false);
    this->setCursor(Qt::WaitCursor);

    QString pin = ui->pinEdit->text();
    std::string login_request = "login_" + pin.toStdString();
    send(SEcube_socket, login_request.c_str(), login_request.length(), 0);

    char server_response[32] = {0};
    int res = read(SEcube_socket, server_response, 31);
    if (res < 0) {
        callErrorDialog("Error in receiving server response", "Please, try again");
    } else {
        server_response[res] = 0;
    }

    if (strcmp(server_response, "3") == 0) {
        callErrorDialog("Error in sending login request", "Please, try again");
    }
    else if (strcmp(server_response, "2") == 0) {
        callErrorDialog("Error in reading login request", "Please, try again");
    }
    else if (strcmp(server_response, "err_nosec") == 0) {
        callErrorDialog("No SEcube connected",
                        "Please, connect your SEcube and try again");
    }
    else if (strcmp(server_response, "err_toomany") == 0) {
        callErrorDialog("More than one SEcube device connected",
                        "Please, connect only your SEcube and try again");
    }
    else if (strcmp(server_response, "err_login") == 0) {
        callErrorDialog("Error in performing login", "Please, try again");
    }
    else if (strcmp(server_response, "err_sekey") == 0) {
        callErrorDialog("Error in starting SEkey", "Please, try again");
    }
    else if (strcmp(server_response, "err_db") == 0) {
        callErrorDialog("Error opening the database containing Telegram - SEkey associations", "Please, try again");
    }
    else if (strcmp(server_response, "setup_ok") == 0) {
        setup_ok = true;
        ui->enterLabel->hide();
        ui->pinEdit->hide();
        ui->accessButton->hide();
        ui->loadingLabel->show();

        // start the telegram thread (manages communication with telegram server)
        logmessage("Start the telegram thread");
        telegram_thread = std::thread([]() {
            Td td;
            td.loop();
        });

        while (true) {
            // Login to Telegram
            {
                std::lock_guard<std::mutex> lk(Td::mutex_ready);
                Td::thread_ready = true;
            }
            Td::cv_ready.notify_one();

            std::unique_lock<std::mutex> u_lk(Td::mutex_);
            Td::cv_.wait(u_lk, [] { return Td::action_sent; });
            Td::action_sent = false;

            if (Td::action == "login_done") {
                u_lk.unlock();
                { // Telegram thread is waiting on thread_ready, wake it
                    std::lock_guard<std::mutex> lk(Td::mutex_ready);
                    Td::thread_ready = true;
                }
                Td::cv_ready.notify_one();
                break;
            } else {
                if (Td::action == "key") {
                    auto telegram_dialog = new TelegramAccessDialog(
                            nullptr, "Enter Telegram encryption key (or DESTROY)");
                    if (telegram_dialog->exec() == QDialog::Accepted) {
                        key = telegram_dialog->input;
                    }
                }
                else if (Td::action == "phone") {
                    auto telegram_dialog = new TelegramAccessDialog(
                            nullptr, "Enter phone number (with prefix)");
                    if (telegram_dialog->exec() == QDialog::Accepted) {
                        phone = telegram_dialog->input;
                    }
                }
                else if (Td::action == "code") {
                    auto telegram_dialog = new TelegramAccessDialog(
                            nullptr, "Enter Telegram authentication code");
                    if (telegram_dialog->exec() == QDialog::Accepted) {
                        code = telegram_dialog->input;
                    }
                }
                input_wrote = true;
                u_lk.unlock();
                Td::cv_.notify_one();
            }
        }

        cw = new ChatWindow();
        cw->show();
        this->close();
    }

    this->setCursor(Qt::ArrowCursor);
    ui->accessButton->setEnabled(true);
}

