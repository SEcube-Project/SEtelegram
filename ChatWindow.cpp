/**
  ******************************************************************************
  * File Name          : ChatWindow.cpp
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

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif
#include "ChatWindow.h"
#include "ui_ChatWindow.h"
#include <iostream>
#include "Frames/DocumentMessageFrame.h"
#include "Frames/ChatButtonFrame.h"
#include "Td.h"
#include <QStyle>
#include <QDesktopWidget>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QLabel>
#include <QCloseEvent>
#include <cstdint>
#include "global.h"
#include <string>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <QSizePolicy>
#include "Dialogs/AttachmentDialog.h"
#include "Frames/ImageMessageFrame.h"
#include "Frames/TextMessageFrame.h"

#define DEFAULT_BUFLEN 1024*1024*10
#define CHAT_FETCH_UPDATE_INTERVAL 30 // seconds

bool dont_move_scrollbar = false; // flag used to avoid moving the scrollbar to the bottom when loading previous messages
// these two functions are used to compare telegram messages depending on their date attribute (useful when sorting messages)
bool messagecomparator(const td::tl::unique_ptr<td_api::message>& p1, const td::tl::unique_ptr<td_api::message>& p2);
bool messagecomparator_reverse(const td::tl::unique_ptr<td_api::message>& p1, const td::tl::unique_ptr<td_api::message>& p2);

int enc_or_dec(const std::string& e_or_d, const std::string& data_type, std::string& path_or_data, std::string& result);

bool messagecomparator(const td::tl::unique_ptr<td_api::message>& p1, const td::tl::unique_ptr<td_api::message>& p2){
    if(p1->date_ < p2->date_){
        return true;
    }
    return false;
}
bool messagecomparator_reverse(const td::tl::unique_ptr<td_api::message>& p1, const td::tl::unique_ptr<td_api::message>& p2){
    if(p1->date_ < p2->date_){
        return false;
    }
    return true;
}

/** Create main GUI window to show chat list and messages */
ChatWindow::ChatWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::ChatWindow) {
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,this->size(), qApp->desktop()->availableGeometry()));
    ui->textMessageEdit->setAlignment(Qt::AlignVCenter);
    ui->inputWidget->hide();
    char buff[FILENAME_MAX];
    GetCurrentDir(buff, FILENAME_MAX);
    std::string current_working_dir(buff);
    download_folder = current_working_dir + "/tdlib/";
    waitTelegram(); // wait until Telegram thread is ready to process GUI requests
    std::string action_str = "startup";
    requestTelegram_startup(action_str); // request startup to Telegram thread (internally it waits for results from Telegram thread)
    // process results received from Telegram thread
    if (Td::error) {
        callErrorDialog("Initialization error");
        this->close();
        return;
    }
    Td::ok = false;
    auto * layout = new QVBoxLayout();
    layout->addWidget(ui->chatsScrollArea);
    auto * chats_container = new QWidget();
    ui->chatsScrollArea->setWidget(chats_container);
    layout = new QVBoxLayout(chats_container);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    // populate chat list on the left of the GUI
    std::for_each(Td::chat_list.begin(), Td::chat_list.end(),[this, layout, chats_container] (const std::pair<long, SEchat>& chat) {
        auto chat_button = new ChatButtonFrame(chats_container, chat.second.chat_name, chat.second.unread_count);
        chat_buttons.insert(std::pair<long,ChatButtonFrame*>(chat.first, chat_button));
        layout->addWidget(chat_button);
        connect(chat_button, &ChatButtonFrame::clicked,[this, chat](){ chatButtonClicked(chat.first); });
    });
//    messagesLayout = new QVBoxLayout();
//    messagesLayout->addWidget(ui->messagesScrollArea);
    messages_container = new QWidget();
    messages_container->setStyleSheet("QWidget {background-color : transparent;}");
    ui->messagesScrollArea->setWidget(messages_container);
    messagesLayout = new QVBoxLayout(messages_container);
    ui->messagesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->messagesScrollArea->horizontalScrollBar()->setEnabled(false);
    selezionaChatLabel = new QLabel("Please select a chat to start messaging");
    QFile File(":/css/stylesheet.qss");
    File.open(QFile::ReadOnly);
    QString StyleSheet = QLatin1String(File.readAll());
    selezionaChatLabel->setStyleSheet(StyleSheet);
    selezionaChatLabel->setObjectName("selezionaChatLabel");
    selezionaChatLabel->setMaximumHeight(36);
    messagesLayout->setSizeConstraint(QLayout::SetMinimumSize);
    messagesLayout->addWidget(selezionaChatLabel);
    messagesLayout->setAlignment(selezionaChatLabel, Qt::AlignHCenter);
    selected_chat = { 0, "", 0, 0, 0, "" };
    connect(this, &ChatWindow::checkNewMessages,this, &ChatWindow::newMessagesUpdate);
    auto *keyPressEater = new KeyPressEater();
    QCoreApplication::instance()->installEventFilter(keyPressEater);
    // launch the thread responsible for updating the GUI view each 30 seconds
    update_thread = std::thread([this]() {
        while(wait_for(std::chrono::seconds(CHAT_FETCH_UPDATE_INTERVAL))) {
            emit checkNewMessages(); // this signal makes the GUI thread execute the newMessagesUpdate method
        }
    });
}

/** Select a new chat and show the messages */
void ChatWindow::chatButtonClicked(long chat_id) { // chat_id is the id of the selected chat
    if(Td::chat_list.find(chat_id)->second.is_SEcube_chat == -1) { // the SEcube does not have any rule for this chat, error
        QMessageBox errormessage;
        errormessage.setIcon(QMessageBox::Critical);
        errormessage.setText("Error, the selected user is not correctly configured.");
        errormessage.setInformativeText("Try to restart the application in order to solve the problem.");
        errormessage.setStandardButtons(QMessageBox::Ok);
        errormessage.setDefaultButton(QMessageBox::Ok);
        errormessage.exec();
        return;
    }
    logmessage("SELECTED CHAT: " + std::to_string(chat_id));
    if (chat_id == selected_chat.chat_id) { // selected_chat.chat_id is the id of the chat that was selected before
        logmessage("CHAT ALREADY SELECTED: " + std::to_string(chat_id));
        return;
    }
    disconnect(ui->messagesScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &ChatWindow::getPreviousMessages);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    closed_chat_id = selected_chat.chat_id;
    if (closed_chat_id != 0) { // a chat was selected before, so close it
        chat_buttons.find(closed_chat_id)->second->selected(false);
    }
    selected_chat = Td::chat_list.find(chat_id)->second; // update currently selected chat
    logmessage(selected_chat.chat_name + " " + std::to_string(selected_chat.chat_id) + " " + std::to_string(selected_chat.is_SEcube_chat));
    chat_buttons.find(chat_id)->second->selected(true);
    waitTelegram(); // wait for Telegram thread to be ready to process GUI requests
    Td::messages.clear();
    std::string action_str = "chat_messages";
    requestTelegram(action_str); // get messages of selected chat
    messages_container = new QWidget();
    messages_container->setStyleSheet("QWidget {background-color : transparent;}");
    ui->messagesScrollArea->setWidget(messages_container);
    messagesLayout = new QVBoxLayout(messages_container);
    if (Td::error) {
        callErrorDialog("Error in retrieving messages");
        Td::error = false;
    }
    else {
        std::sort(Td::messages.begin(), Td::messages.end(), messagecomparator);
        for (auto &message : Td::messages) {
            showMessage(message, false);
        }
        Td::ok = false;
    }
    if (!Td::error) {
        showImages();
        auto scrollbar = ui->messagesScrollArea->verticalScrollBar();
        connect(scrollbar, &QScrollBar::rangeChanged, this, &ChatWindow::setScrollbarPosition);
        connect(scrollbar, &QScrollBar::valueChanged, this, &ChatWindow::getPreviousMessages);
    }
    if (ui->inputWidget->isHidden()) {
        ui->inputWidget->show();
    }
    if (!ui->textMessageEdit->toPlainText().isEmpty()) {
        ui->textMessageEdit->clear();
    }
    connect(ui->sendButton, &QPushButton::clicked, [this](){sendButtonClicked();});
    connect(ui->attachmentButton, /*&QPushButton::clicked*/ &QPushButton::pressed, [this](){attachmentButtonClicked();});
    QApplication::restoreOverrideCursor();
}

/** Move scrollbar to the bottom of the page */
void ChatWindow::setScrollbarPosition() {
    if(!dont_move_scrollbar){ // don't move scrollbar when flag is set to true (happens only when retrieving old messages in chat)
        ui->messagesScrollArea->verticalScrollBar()->setValue(ui->messagesScrollArea->verticalScrollBar()->maximum());
    } else {
        dont_move_scrollbar = false;
    }
}

/** Show previous messages in the chat history */
void ChatWindow::getPreviousMessages() {
    if (ui->messagesScrollArea->verticalScrollBar()->value() != ui->messagesScrollArea->verticalScrollBar()->minimum()) {
        return;
    }
    auto scrollbar = ui->messagesScrollArea->verticalScrollBar();
    disconnect(scrollbar, &QScrollBar::rangeChanged, this, &ChatWindow::setScrollbarPosition);
    Td::previous_messages.clear();
    waitTelegram();
    std::string action_str = "previous_messages";
    requestTelegram(action_str);
    if (Td::error) {
        callErrorDialog("Error in retrieving messages");
        Td::error = false;
    } else {
        std::vector<std::int64_t> keep;
        std::sort(Td::previous_messages.begin(), Td::previous_messages.end(), messagecomparator_reverse);
        for (auto &message : Td::previous_messages) {
            bool duplicate = false;
            for(auto& msg_ : Td::messages){
                if(message->id_ == msg_->id_){
                    duplicate = true;
                    break;
                }
            }
            if(!duplicate){
                for(auto x : keep){
                    if(message->id_ == x){
                        duplicate = true;
                        break;
                    }
                }
            }
            if(!duplicate){
                keep.push_back(message->id_);
                showMessage(message, true);
            }
        }
        // Lambda for removing messages that must be ignored
        auto remover = [&](td::tl::unique_ptr<td_api::message>& element) -> bool {
            for(int i=0; i<keep.size(); i++){
                if(keep[i] == element->id_){
                    keep.erase(keep.begin()+i); // erase to avoid keeping multiple copies
                    return false;
                }
            }
            return true;
        };
        std::remove_if(Td::previous_messages.begin(), Td::previous_messages.end(), remover);
        Td::messages.insert(Td::messages.begin(), std::make_move_iterator(Td::previous_messages.begin()), std::make_move_iterator(Td::previous_messages.end()));
        Td::ok = false;
        showImages();
    }
    Td::previous_messages.clear();
    connect(scrollbar, &QScrollBar::rangeChanged, this, &ChatWindow::setScrollbarPosition);
    dont_move_scrollbar = true;
}

/** Download the document contained in the selected message */
void ChatWindow::documentButtonClicked(std::int32_t file_id,const std::string& file_name) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    std::string downloaded_file_path = download_folder + "documents/" + file_name;
    std::ifstream f(downloaded_file_path.c_str());
    if (f.good()) {
        f.close();
        if (selected_chat.is_SEcube_chat == 1) {
            std::string dec_result;
            int res = enc_or_dec("d", "file", downloaded_file_path, dec_result);
            if (res == 1) {
                callErrorDialog("Error in decrypting file");
            }
            else {
                QDesktopServices::openUrl(QUrl::fromLocalFile(dec_result.c_str()));
            }
        }
        else if (selected_chat.is_SEcube_chat == 0) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(downloaded_file_path.c_str()));
        }
    }
    else {
        logmessage("DOWNLOADING FILE (ID: " + std::to_string(file_id) + ")");
        waitTelegram();
        std::string action_str = "file_" + std::to_string(file_id);
        requestTelegram(action_str);
        if (Td::error) {
            callErrorDialog("Error in downloading file");
            Td::error = false;
        }
        else {
            Td::ok = false;
            if (selected_chat.is_SEcube_chat == 1) {
                std::string dec_result;
                int res = enc_or_dec("d", "file",Td::file->local_->path_, dec_result);
                if (res == 1) {
                    callErrorDialog("Error in decrypting file");
                }
                else if (selected_chat.is_SEcube_chat == 0) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(dec_result.c_str()));
                }
            }
            else {
                QDesktopServices::openUrl(QUrl::fromLocalFile(Td::file->local_->path_.c_str()));
            }
        }
    }
    QApplication::restoreOverrideCursor();
}

/** Send a message containing a document or an image */
void ChatWindow::attachmentButtonClicked() {
    ui->attachmentButton->setEnabled(false);
    attachmentPath = "";
    attachmentPath = QFileDialog::getOpenFileName(
            this, tr("Choose file or image"),
            "~", tr("")).toStdString();
    if (!attachmentPath.empty()) {
        std::size_t found = attachmentPath.find_last_of("/\\");
        std::string file_name = attachmentPath.substr(found+1);
        logmessage("Attachment file: " + file_name);
        auto dialog = new AttachmentDialog(nullptr, file_name);
        dialog->setModal(true);
        if(dialog->exec() == QDialog::Accepted) {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            std::ifstream file(attachmentPath, std::ios::binary);
            std::string caption = dialog->getCaption();
            std::string copy_path;
            bool photo = false;
            bool plain_photo = false;

            if (attachmentPath.find(".jpg") != std::string::npos ||
                attachmentPath.find(".png") != std::string::npos ||
                attachmentPath.rfind(".jpeg") != std::string::npos) {
                photo = true;
                copy_path = download_folder + "photos/" + file_name;
                if (selected_chat.is_SEcube_chat == 0) {
                    plain_photo = true;
                }
            }
            else {
                copy_path = download_folder + "documents/" + file_name;
            }
            logmessage("NEW POSITION: " + copy_path);
            std::ofstream file_copy(copy_path, std::ios::binary);
            file_copy << file.rdbuf();
            if (!file_copy.good()) {
                printerr("Error in copying file");
                copy_path = attachmentPath;
            }

            if (plain_photo) {
                auto message_content = td_api::make_object<td_api::inputMessagePhoto>();
                message_content->photo_ = td_api::make_object<td_api::inputFileLocal>(copy_path);
                message_content->thumbnail_ = nullptr;
                message_content->added_sticker_file_ids_ = std::vector<int32_t>();
                message_content->ttl_ = 0;
                message_content->height_ = 0;
                message_content->width_ = 0;
                if (!caption.empty()) {
                    message_content->caption_ = td_api::make_object<td_api::formattedText>();
                    message_content->caption_->text_ = caption;
                }
                Td::inputMessageContent = std::move(message_content);
            }
            else { // generic plain or encrypted file or encrypted image
                auto message_content = td_api::make_object<td_api::inputMessageDocument>();

                if (selected_chat.is_SEcube_chat == 1) { // encrypted file or image
                    std::string enc_result;
                    int res = enc_or_dec("e", "file", copy_path, enc_result);
                    if (res == 1) {
                        callErrorDialog("Error in encrypting the attachment", "");
                    }
                    else {
                        message_content->document_ = td_api::make_object<td_api::inputFileLocal>(enc_result);
                    }
                }
                else { // plain file
                    message_content->document_ = td_api::make_object<td_api::inputFileLocal>(copy_path);
                }

                message_content->thumbnail_ = nullptr;

                if (!caption.empty()) {
                    message_content->caption_ = td_api::make_object<td_api::formattedText>();
                    if (selected_chat.is_SEcube_chat == 1) {
                        std::string enc_result;
                        int res = enc_or_dec("e", "text", caption, enc_result);
                        if (res == 1) {
                            callErrorDialog("Error in encrypting the caption", "");
                        }
                        else {
                            message_content->caption_->text_ = enc_result;
                        }
                    }
                    else {
                        message_content->caption_->text_ = caption;
                    }
                }
                Td::inputMessageContent = std::move(message_content);
            }
            sendMessage();

            if (Td::error) {
                callErrorDialog("Error in sending message");
                Td::error = false;
            }
            else {
                Td::ok = false;
                if (photo) {
                    auto imf = new ImageMessageFrame(nullptr,"",
                                                     caption, false, true, "");
                    imf->setImage(copy_path);
                    messagesLayout->addWidget(imf);
                    connect(imf, &ImageMessageFrame::clicked,
                            [this, copy_path]() {
                                QDesktopServices::openUrl(QUrl::fromLocalFile(copy_path.c_str()));
                            });
                    messagesLayout->setAlignment(imf, Qt::AlignRight);
                }
                else {
                    auto dmf = new DocumentMessageFrame(nullptr,"",
                                                        file_name, caption, false, true);
                    messagesLayout->addWidget(dmf);
                    connect(dmf, &DocumentMessageFrame::clicked,[this,copy_path](){
                        QDesktopServices::openUrl(QUrl::fromLocalFile(copy_path.c_str())); });
                    messagesLayout->setAlignment(dmf, Qt::AlignRight);
                }
            }

            QApplication::restoreOverrideCursor();
        }
    }

    ui->attachmentButton->setEnabled(true);
}

/** Send a text-only message */
void ChatWindow::sendButtonClicked() {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    std::string text = ui->textMessageEdit->toPlainText().toStdString();
    if (!text.empty()) {
        auto message_content = td_api::make_object<td_api::inputMessageText>();
        message_content->text_ = td_api::make_object<td_api::formattedText>();
        // if chat is encrypted, encrypt the message
        if (selected_chat.is_SEcube_chat == 1) {
            std::string enc_result;
            int res = enc_or_dec("e", "text", text, enc_result);
            if (res == 1) {
                callErrorDialog("Error in encrypting the message", "");
            }
            else {
                message_content->text_->text_ = enc_result;
            }
        } else { // chat is not encrypted
            message_content->text_->text_ = text;
        }

        Td::inputMessageContent = std::move(message_content);
        sendMessage();

        ui->textMessageEdit->clear();
        auto tmf = new TextMessageFrame(nullptr,"",text,
                                        false, true);
        messagesLayout->addWidget(tmf);
        messagesLayout->setAlignment(tmf, Qt::AlignRight);
    }

    QApplication::restoreOverrideCursor();
}

ChatWindow::~ChatWindow() {
    delete ui;
}

void ChatWindow::closeEvent(QCloseEvent *event) {
    logmessage("Joining update thread");
    {
        std::lock_guard<std::mutex> l(update_stop_mutex);
        update_stop = true;
    }
    update_stop_cv.notify_one();
    update_thread.join();
    logmessage("Update thread joined");

    logmessage("Joining telegram thread");
    waitTelegram();

    Td::action = "quit";
    logmessage("Send quit request to the telegram thread");
    {
        std::lock_guard<std::mutex> lk(Td::mutex_);
        Td::action_sent = true;
    }
    logmessage("Notify the telegram thread");
    Td::cv_.notify_one();
    telegram_thread.join();
    logmessage("Telegram thread joined");

    logmessage("Joining SEcube thread");
    send(SEcube_socket, "quit", 4, 0);
    ::close(SEcube_socket);
    secube_thread.join();
    logmessage("SEcube thread joined");

    logmessage("Now closing");
}

/** Method for showing a message using a custom frame according to its type
 *  - TextMessageFrame for texts
 *  - DocumentMessageFrame for documents
 *  - ImageMessageFrame for images */
void ChatWindow::showMessage(td::tl::unique_ptr<td::td_api::message> & message, bool put_at_top) {
    std::string sender_name;
    bool is_from_me = false;

    if (message->sender_user_id_ == Td::my_telegram_id) {
        is_from_me = true;
    }
    else {
        if (selected_chat.chat_type == td_api::chatTypeBasicGroup::ID ||
            selected_chat.chat_type == td_api::chatTypeSupergroup::ID) {
            sender_name = Td::get_user_name(message->sender_user_id_);
        }
    }

    switch(message->content_->get_id()) {

        case td_api::messageText::ID : {
            auto messageText = td_api::move_object_as<td_api::messageText>(message->content_);
            std::string text = messageText->text_->text_;
            bool error = false;

            if (selected_chat.is_SEcube_chat == 1) {
                std::string dec_result;
                int res = enc_or_dec("d", "text", text, dec_result);
                if (res == 1) {
                    text = "Error in decrypting message";
                    error = true;
                }
                else {
                    text = dec_result;
                }
            }
            auto tmf = new TextMessageFrame(nullptr,sender_name,
                                            text,error,is_from_me);
            if (put_at_top) {
                if(messagesLayout->isEmpty()){
                    messagesLayout->addWidget(tmf);
                } else {
                    messagesLayout->insertWidget(0, tmf);
                }
            }
            else {
                messagesLayout->addWidget(tmf);
            }
            if (is_from_me) {
                messagesLayout->setAlignment(tmf, Qt::AlignRight);
            }
            else {
                messagesLayout->setAlignment(tmf, Qt::AlignLeft);
            }
            break;
        }

        case td_api::messageDocument::ID : {
            auto messageDocument = td_api::move_object_as<td_api::messageDocument>(message->content_);
            std::int32_t file_id = messageDocument->document_->document_->id_;
            auto file_name = messageDocument->document_.get()->file_name_;
            logmessage("File name: " + file_name);

            auto caption = messageDocument->caption_->text_;
            bool caption_err = false;
            if (!caption.empty()) {
                if (selected_chat.is_SEcube_chat == 1) {
                    std::string dec_result;
                    int res = enc_or_dec("d", "text", caption, dec_result);
                    if (res == 1) {
                        printerr("Caption: Couldn't decrypt caption");
                        caption = "Error in decrypting the caption";
                        caption_err = true;
                    }
                    else {
                        logmessage("Caption: " + dec_result);
                        caption = dec_result;
                    }
                }
            }

            if (file_name.find(".jpg") != std::string::npos ||
                file_name.find(".png") != std::string::npos ||
                file_name.rfind(".jpeg") != std::string::npos) {
                // encrypted image
                auto imf = new ImageMessageFrame(nullptr, sender_name,
                                                 caption, caption_err, is_from_me, std::to_string(file_id));
                if (put_at_top) {
                    messagesLayout->insertWidget(0, imf);
                }
                else {
                    messagesLayout->addWidget(imf);
                }
                if (is_from_me) {
                    messagesLayout->setAlignment(imf, Qt::AlignRight);
                }
                else {
                    messagesLayout->setAlignment(imf, Qt::AlignLeft);
                }
                image_messages.insert(image_messages.begin(), imf);
            }
            else { // generic file (plain or encrypted)
                auto dmf = new DocumentMessageFrame(nullptr, sender_name,
                        file_name, caption, caption_err, is_from_me);
                if (put_at_top) {
                    messagesLayout->insertWidget(0, dmf);
                }
                else {
                    messagesLayout->addWidget(dmf);
                }
                connect(dmf, &DocumentMessageFrame::clicked,
                        [this, file_id, file_name](){
                            documentButtonClicked(file_id, file_name); });
                if (is_from_me) {
                    messagesLayout->setAlignment(dmf, Qt::AlignRight);
                }
                else {
                    messagesLayout->setAlignment(dmf, Qt::AlignLeft);
                }
            }
            break;
        }

        case td_api::messagePhoto::ID : {
            auto messagePhoto = td_api::move_object_as<td_api::messagePhoto>(message->content_);
            std::int32_t photo_id = messagePhoto->photo_->sizes_.back()->photo_->id_;
            auto caption = messagePhoto->caption_->text_;

            bool caption_err = false;
            if (!caption.empty()) {
                if (selected_chat.is_SEcube_chat == 1) {
                    std::string dec_result;
                    int res = enc_or_dec("d", "text", caption, dec_result);
                    if (res == 1) {
                        printerr("Caption: Couldn't decrypt caption");
                        caption = "Error in decrypting the caption";
                        caption_err = true;
                    }
                    else {
                        logmessage("Caption: " + dec_result);
                        caption = dec_result;
                    }
                }
            }

            auto imf = new ImageMessageFrame(nullptr, sender_name,
                                             caption, caption_err, is_from_me, std::to_string(photo_id));
            if (put_at_top) {
                messagesLayout->insertWidget(0, imf);
            }
            else {
                messagesLayout->addWidget(imf);
            }
            if (is_from_me) {
                messagesLayout->setAlignment(imf, Qt::AlignRight);
            }
            else {
                messagesLayout->setAlignment(imf, Qt::AlignLeft);
            }
            image_messages.insert(image_messages.begin(), imf);
            break;
        }

        default:
            break;
    }
}

/** Method to show new incoming messages each minute */
void ChatWindow::newMessagesUpdate() {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    logmessage("=================== UPDATE: Check for new messages ===================");
    Td::newMessages.clear();
    waitTelegram();
    std::string action_str = "update";
    requestTelegram(action_str);
    Td::ok = false;
    std::vector<std::int64_t> keep;
    std::sort(Td::newMessages.begin(), Td::newMessages.end(), messagecomparator);
    if (selected_chat.chat_id != 0) {
        for (auto & new_message : Td::newMessages) {
            if(new_message->chat_id_ != selected_chat.chat_id){ // ignore messages that do not belong to the currently selected chat
                continue;
            }
            bool already_displayed = false;
            for (auto & message : Td::messages) {
                if (new_message->id_ == message->id_) {
                    already_displayed = true;
                    break;
                }
            }
            if(!already_displayed){
                for(auto x : keep){
                    if(new_message->id_ == x){
                        already_displayed = true;
                        break;
                    }
                }
            }
            if (!already_displayed) {
                showMessage(new_message, false);
                keep.push_back(new_message->id_);
            }
        }
        // Lambda for removing messages that must be ignored
        auto remover = [&](td::tl::unique_ptr<td_api::message>& element) -> bool {
            for(int i=0; i<keep.size(); i++){
                if(keep[i] == element->id_){
                    keep.erase(keep.begin()+i); // erase to avoid keeping multiple copies
                    return false;
                }
            }
            return true;
        };
        std::remove_if(Td::newMessages.begin(), Td::newMessages.end(), remover);
        Td::messages.insert(Td::messages.end(), std::make_move_iterator(Td::newMessages.begin()), std::make_move_iterator(Td::newMessages.end()));
        Td::newMessages.clear();
        showImages();
    }
    logmessage("=================== UPDATE: done ===================");
    QApplication::restoreOverrideCursor();
}

template<class Duration>
bool ChatWindow::wait_for(Duration duration) {
    std::unique_lock<std::mutex> l(update_stop_mutex);
    return !update_stop_cv.wait_for(l, duration, []() { return update_stop; });
}

void ChatWindow::sendMessage() {
    waitTelegram();
    std::string action_str = "send_message";
    requestTelegram(action_str);
}

/** Method to request to the SEcube to encrypt or decrypt data */
int enc_or_dec(const std::string& e_or_d, const std::string& data_type, std::string& path_or_data, std::string& result) {
    std::string server_str;
    std::string magicstr("ilN1yZ91JLcQ8tYx5i2l8WqssBqp6r5Q0L2wTg7iSWYHMUhZvKZhI4NyL3PT");
    if (e_or_d == "e") { // encrypt_groupid_datatype_data
        server_str = "encrypt_" + std::to_string(selected_chat.chat_id) + "_" + data_type + "_" + path_or_data;
    }
    else if (e_or_d == "d") { // decrypt_datatype_data
        if(data_type == "text" && path_or_data.length()>=magicstr.length() && path_or_data.compare(0, magicstr.length(), magicstr)==0){
            server_str = "decrypt_" + data_type + "_" + path_or_data.substr(magicstr.length(), std::string::npos); // decryption required
        } else if(data_type == "file" && path_or_data.compare(path_or_data.length()-4, 4, ".enc")==0){
            server_str = "decrypt_" + data_type + "_" + path_or_data; // decryption required
        } else {
            result = path_or_data; // decryption not required
            return 0;
        }

    }
    send(SEcube_socket, server_str.c_str(), server_str.length(), 0);

    // First receive 0 or 1, whether the SEcube operation went ok or not
    // Server response: 0_data (success), 1 (error)

    //char server_res[DEFAULT_BUFLEN] = {0};
    std::unique_ptr<char[]> server_res = std::make_unique<char[]>(DEFAULT_BUFLEN);
    int res = read(SEcube_socket, server_res.get(), DEFAULT_BUFLEN-1);
    if (res < 0) {
        printerr("Error in receiving server response (1)");
        return 1;
    }
    server_res[res] = 0;
    std::string server_res_str(server_res.get());
    if (server_res[0] == '1') {
        std::string tmps = "Error in encryption or decryption: ";
        tmps.append(strerror(errno));
        printerr(tmps);
        return 1;
    }
    if (server_res[0] != '0') {
        printerr("Error in receiving server response (2) ");
        return 1;
    }

    if (server_res[1] != '_') {
        printerr("Error in receiving server response (3) ");
        return 1;
    }
    result = server_res_str.substr(2, server_res_str.length());
    if(e_or_d == "e"){ // if encryption, prepend a "magic value" to identify this as encrypted payload
        if(data_type == "text"){
            result.insert(0, magicstr);
        }
    }
    return 0;
}

/** Method to download and show the images contained in the visualized messages */
void ChatWindow::showImages() {
    for (auto image : image_messages) {
        waitTelegram();
        std::string action_str = "file_" + image->get_image_id();
        requestTelegram(action_str);
        if (Td::error) {
            image->setImage("../resources/errore_img.png");
            Td::error = false;
        }
        else {
            Td::ok = false;
            auto image_path = Td::file->local_->path_;
            if (selected_chat.is_SEcube_chat == 1) {
                std::string dec_result;
                int res = enc_or_dec("d", "file", image_path, dec_result);
                if (res == 1) {
                    image->setImage("../resources/errore_img.png");
                }
                else {
                    image_path = dec_result; // in "documents" directory
                    logmessage("IMAGE DECRYPTED: " + image_path);
                    std::ifstream image_file(image_path, std::ios::binary);
                    auto found = image_path.find_last_of("/\\");
                    std::string image_name = image_path.substr(found+1);
                    logmessage("IMAGE NAME: " + image_name);
                    std::string copy_path = download_folder + "photos/" + image_name;
                    logmessage("IMAGE NEW PATH: " + copy_path);

                    std::ifstream f(copy_path, std::ios::binary);
                    if (f.good()) { // image already exists in "photos" directory
                        f.close();
                    }
                    else { // copy image in "photos" directory
                        std::ofstream image_file_copy(copy_path, std::ios::binary);
                        image_file_copy << image_file.rdbuf();
                        if (!image_file_copy.good()) {
                            printerr("Error in copying file");
                            copy_path = dec_result;
                        }
                    }
                    image->setImage(copy_path);
                }
            }
            else {
                image->setImage(image_path);
            }

            if (image->image_path != "../resources/errore_img.png") {
                connect(image, &ImageMessageFrame::clicked,
                        [image_path]() {
                            QDesktopServices::openUrl(QUrl::fromLocalFile(image_path.c_str()));
                        });
            }
        }
    }
    image_messages.clear();
}

/** Method to wait for the telegram thread to be ready to receive requests */
void ChatWindow::waitTelegram() {
    logmessage("Wait for the telegram thread to be ready to receive requests");
    {
        std::unique_lock<std::mutex> u_lk(Td::mutex_ready);
        Td::cv_ready.wait(u_lk, []{return Td::thread_ready;});
        Td::thread_ready = false;
    }
}

/** Method to request data or another action to the telegram thread */
void ChatWindow::requestTelegram(std::string& action_str) {
    Td::action = action_str;
    {
        std::lock_guard<std::mutex> lk(Td::mutex_);
        Td::action_sent = true; // send request to the telegram thread
    }
    Td::cv_.notify_one();
    {
        std::unique_lock<std::mutex> u_lk(Td::mutex_);
        Td::cv_.wait(u_lk, []{return Td::ok || Td::error;}); // wait for result of request to telegram thread
        /* TODO: an alternative implementation of this function is to modify the return type from void to bool.
         *       The returned value is true if Td::ok is true after the wait on the cv, false otherwise.
         *       In this way, the calling function does not need to manually reset the flags Td::ok and Td::error
         *       to false every single time, because they are already reset here before returning.
         *       The calling function, however, must check the value returned by requestTelegram(). */
    }
}

/** This method is the same as the one above, it just has one additional synchronization step in the middle
 *  that is required to avoid the deadlock between the GUI and the Telegram thread, which may happen in a particular case. */
void ChatWindow::requestTelegram_startup(std::string& action_str) {
    Td::action = action_str;
    {
        std::lock_guard<std::mutex> lk(Td::mutex_);
        Td::action_sent = true; // send request to the telegram thread
    }
    Td::cv_.notify_one();
    while(true){
        std::unique_lock<std::mutex> u_lk(Td::mutex_startup);
        // wait for intermediate results in case GUI interaction is needed when retrieving new chats at startup
        Td::cv_startup.wait(u_lk, []{return Td::startup;});
        Td::startup = false;
        if(Td::chat_list_done){
           Td::chat_list_done = false;
           break;
        } else {
             QMessageBox msgbox;
             std::string tmps = "Encrypt chat with <b>" + Td::chatName + "</b> ?";
             msgbox.setText(tmps.c_str());
             msgbox.setInformativeText("The SEcube device provides end-to-end encryption.");
             msgbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
             msgbox.setDefaultButton(QMessageBox::No);
             msgbox.setDetailedText("This dialog is prompted only when Telegram opens a chat that is not"
                                    " yet associated to the SEcube, independently if you choose to encrypt the chat"
                                    " or not.");
             msgbox.setIcon(QMessageBox::Question);
             int ret = msgbox.exec();
             if(ret == QMessageBox::Yes){ // encryption required
                 Td::useSEcube = true;
                 Td::SEkeyID = "";
                 bool inputdialogok = false;
                 QString inputdialogtext = QInputDialog::getText(this, tr("Enter ID"),
                                                      tr("Enter SEkey user ID or SEkey group ID:"), QLineEdit::Normal,
                                                      "i.e. U1 or G1", &inputdialogok);
                 if(inputdialogok && !inputdialogtext.isEmpty()){
                     Td::SEkeyID = inputdialogtext.toStdString();
                 }
             } else { // encryption not required
                 Td::useSEcube = false;
                 Td::SEkeyID = "";
             }
            Td::chatName.clear(); // reset shared variable
            {
                std::lock_guard<std::mutex> startup_lk(Td::mutex_startup2);
                Td::startup2 = true;
            }
            Td::cv_startup2.notify_one();
        }
    }
    {
        std::unique_lock<std::mutex> u_lk(Td::mutex_);
        Td::cv_.wait(u_lk, []{return Td::ok || Td::error;}); // wait for result of request to telegram thread
        /* TODO: an alternative implementation of this function is to modify the return type from void to bool.
         *       The returned value is true if Td::ok is true after the wait on the cv, false otherwise.
         *       In this way, the calling function does not need to manually reset the flags Td::ok and Td::error
         *       to false every single time, because they are already reset here before returning.
         *       The calling function, however, must check the value returned by requestTelegram(). */
    }
}
