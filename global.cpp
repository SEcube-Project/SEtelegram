/**
  ******************************************************************************
  * File Name          : global.cpp
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

#include "global.h"
#include <thread>
#include <iostream>
#include "Dialogs/ErrorDialog.h"
#include <QEvent>
#include <fstream>

// this is the path of the executable, compiled with Eclipse, which handles the communication with the SEcube device
std::string SEcubeServer_path = "/home/matteo/Scrivania/SEtelegram/SEcube/SEcubeServer/Release/SEcubeServer.exe";

// this is the port used to communicate between the GUI and the SEcube server
int port = 1235;

#ifdef ENABLE_LOGGING
// this is where the error log is written, configure it with the path that you prefer
std::ofstream errlog("/home/matteo/Scrivania/telegram_errors_log.txt");
// this is where the standard log (errors excluded) is written, configure it with the path that you prefer
std::ofstream telegramlog("/home/matteo/Scrivania/telegram_log.txt");
#endif

SEchat selected_chat;
long closed_chat_id;
std::unordered_map<long,ChatButtonFrame*> chat_buttons;

std::string key;
std::string phone;
std::string code;
bool input_wrote = false;

int SEcube_socket;
std::thread secube_thread;

std::thread update_thread;
std::condition_variable update_stop_cv;
std::mutex update_stop_mutex;
bool update_stop = false;

std::thread telegram_thread;

std::string download_folder;

void printerr(std::string msg){
#ifdef ENABLE_LOGGING
    if(!msg.empty()){
        std::time_t result = std::time(nullptr);
        std::string tmp = std::ctime(&result);
        errlog << tmp.substr(0, tmp.length()-1) << " | " << msg << std::endl;
    }
#endif
}

void logmessage(std::string msg){
#ifdef ENABLE_LOGGING
    if(!msg.empty()){
        std::time_t result = std::time(nullptr);
        std::string tmp = std::ctime(&result);
        telegramlog << tmp.substr(0, tmp.length()-1) << " | " << msg << std::endl;
    }
#endif
}

void callErrorDialog(const std::string& error, const std::string& hint) {
    ErrorDialog ed;
    ed.setModal(true);
    ed.setErrorLabel(error);
    ed.setHintLabel(hint);
    ed.exec();
}

int textWrap(std::string str, std::string& wrapped_str, const QFontMetrics& fm, int max_px_dim, int& longest_size) { //QLabel* label
    int num_rows = 0;
    longest_size = 0;
    bool found_nl = false;

    while (fm.width(str.c_str()) > max_px_dim) {
        int max_chars = 0;
        std::string new_string;
        while (fm.width(new_string.c_str()) <= max_px_dim) {
            max_chars++;
            new_string = str.substr(0, max_chars);
            if ((char)new_string.back() == '\n') {
                found_nl = true;
                break;
            }
        }

        if (found_nl) {
            num_rows++;
            auto len = fm.width(new_string.c_str());
            if (len > longest_size) {
                longest_size = len;
            }
            wrapped_str.append(new_string);
            str = str.substr(new_string.length(), str.length());
            found_nl = false;
        }
        else {
            std::string new_line;
            int n = str.rfind(' ', max_chars-1);
            if (n != std::string::npos) {
                str.at(n) = '\n';
                new_line = str.substr(0, n+1);
                str = str.substr(n+1, str.length());
            }
            else {
                new_line = str.substr(0, max_chars);
                new_line.append("\n");
                str = str.substr(max_chars+1, str.length());
            }
            num_rows++;
            auto len = fm.width(new_line.c_str());
            if (len > longest_size) {
                longest_size = len;
            }
            wrapped_str.append(new_line);
        }
    }
    auto l = fm.width(str.c_str());
    if (l > longest_size) {
        longest_size = l;
    }
    wrapped_str.append(str);
//    std::cout << "textWrap func: " << wrapped_str << std::endl;
//    std::cout << "textWrap func: " << num_rows << std::endl;
    num_rows++;

    return num_rows;
}

std::string textOverflow(QLabel* label, const std::string& str, int max_px_dim) {
    std::string new_string;
    int max_chars = 0;
    while (label->fontMetrics().width(new_string.c_str()) <= max_px_dim) {
        max_chars++;
        new_string = str.substr(0, max_chars);
    }
    return str.substr(0, max_chars - 3).append("...");
}


bool busy;
bool KeyPressEater::eventFilter(QObject *obj, QEvent *event) {
    if (busy) {
        //Just bypass key and mouse events
        std::cout << "bypass key and mouse events" << std::endl;
        switch(event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
            case QEvent::MouseButtonDblClick:
            case QEvent::Resize:
            case QEvent::ScrollPrepare:
            case QEvent::Scroll:
                return true;
        }
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}