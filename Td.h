/**
  ******************************************************************************
  * File Name          : Td.h
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

#ifndef SETELEGRAM_TD_H
#define SETELEGRAM_TD_H

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <QThread>
#include <mutex>
#include <condition_variable>
#include "global.h"

namespace td_api = td::td_api;

/** Class for the management of the communication between the application
 *  and the Telegram servers. Runs in a separate thread ('telegram_thread')*/

class Td {

public:
    Td();
    void loop();

    static std::mutex mutex_;
    static std::condition_variable cv_;
    static std::mutex mutex_ready;
    static std::condition_variable cv_ready;
    static bool thread_ready;
    static std::string action;
    static bool action_sent;

    static bool ok;
    static bool error;

    static std::mutex mutex_startup;
    static std::condition_variable cv_startup;
    static bool startup;
    static std::mutex mutex_startup2;
    static std::condition_variable cv_startup2;
    static bool startup2;
    static bool chat_list_done;
    static std::string chatName;
    static std::string SEkeyID;
    static bool useSEcube;

    static std::int32_t my_telegram_id;

    static std::unordered_map <long, SEchat> chat_list; // map of Telegram chats (chat ID, SEchat object)
    static bool chat_list_received; // this tells if the chat list have been received or not (used on startup)

    static std::vector<td::tl::unique_ptr<td_api::message>> messages;

    static bool chat_closed;
    static bool chat_closed_error;
    static bool chat_opened;
    static bool chat_opened_error;
    static bool messages_viewed;
    static bool messages_viewed_error;

    static std::vector<td::tl::unique_ptr<td_api::message>> previous_messages;

    static td::tl::unique_ptr<td_api::file> file;

    static td::tl::unique_ptr<td_api::InputMessageContent> inputMessageContent;

    static std::vector<td::tl::unique_ptr<td_api::message>> newMessages;

    static std::map<std::int32_t, td_api::object_ptr<td_api::user>> users_;
    static std::string get_user_name(std::int32_t user_id);

private:
    using Object = td_api::object_ptr<td_api::Object>;
    std::unique_ptr<td::Client> client_;

    td_api::object_ptr<td_api::AuthorizationState> authorization_state_;
    bool are_authorized_{false};
    bool need_restart_{false};
    std::uint64_t current_query_id_{0};
    std::uint64_t authentication_query_id_{0};

    std::map<std::uint64_t, std::function<void(Object)>> handlers_;

    void restart();

    void send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler);

    void process_response(td::Client::Response response);

    void process_update(td_api::object_ptr<td_api::Object> update);

    auto create_authentication_query_handler();

    void on_authorization_state_update();

    void check_authentication_error(Object object);

    std::uint64_t next_query_id();

    void waitForServerResponse();

    static void print_response(td::Client::Response& response);
};

#endif //SETELEGRAM_TD_H
