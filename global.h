/**
  ******************************************************************************
  * File Name          : global.h
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

#ifndef SETELEGRAM_GLOBAL_H
#define SETELEGRAM_GLOBAL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <QtWidgets/QLabel>
#include <unordered_map>
#include "Frames/ChatButtonFrame.h"

#define WIDE 500

extern std::string SEcubeServer_path;
extern int port;

struct SEchat {
    long chat_id;
    std::string chat_name;
    int chat_type;
    std::int32_t unread_count;
    int is_SEcube_chat; // 0 (not a SEcube chat), 1 (SEcube chat), -1 (undefined, user needs to decide)
    std::string SEkeyID;
};

extern SEchat selected_chat;
extern long closed_chat_id;
extern std::unordered_map<long,ChatButtonFrame*> chat_buttons;

extern std::string key;
extern std::string phone;
extern std::string code;
extern bool input_wrote;

#define ENABLE_LOGGING
#ifdef ENABLE_LOGGING
extern std::ofstream errlog;
extern std::ofstream telegramlog;
#endif
void printerr(std::string msg);
void logmessage(std::string msg);

extern int SEcube_socket;
extern std::thread secube_thread;
extern std::thread update_thread;
extern std::condition_variable update_stop_cv;
extern std::mutex update_stop_mutex;
extern bool update_stop;

extern std::thread telegram_thread;
extern std::string download_folder;

extern void callErrorDialog(const std::string& error, const std::string& hint = "");

/** Function used in custom frames for messages */
extern int textWrap(std::string str, std::string& wrapped_str, const QFontMetrics& fm,
                    int max_px_dim, int& longest_size);
extern std::string textOverflow(QLabel* label, const std::string& str, int max_px_dim);

extern bool busy;
class KeyPressEater : public QObject {
    Q_OBJECT
    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;
};


#endif //SETELEGRAM_GLOBAL_H
