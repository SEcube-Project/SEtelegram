/**
  ******************************************************************************
  * File Name          : Td.cpp
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

#include "Td.h"
#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <QThread>
#include "global.h"
#include <sys/socket.h>
#include <unistd.h>

// overloaded
namespace detail {
    template <class... Fs>
    struct overload;

    template <class F>
    struct overload<F> : public F {
        explicit overload(F f) : F(f) {
        }
    };
    template <class F, class... Fs>
    struct overload<F, Fs...>
            : public overload<F>
                    , overload<Fs...> {
        overload(F f, Fs... fs) : overload<F>(f), overload<Fs...>(fs...) {
        }
        using overload<F>::operator();
        using overload<Fs...>::operator();
    };
}  // namespace detail

template <class... F>
auto overloaded(F... f) {
    return detail::overload<F...>(f...);
}

namespace td_api = td::td_api;

#define DEFAULT_GETCHATS 50

std::mutex Td::mutex_;
std::condition_variable Td::cv_;
std::mutex Td::mutex_ready;
std::condition_variable Td::cv_ready;
bool Td::thread_ready = false;
std::string Td::action;
bool Td::action_sent = false;

bool Td::ok = false;
bool Td::error = false;

std::mutex Td::mutex_startup;
std::condition_variable Td::cv_startup;
bool Td::startup = false;
std::mutex Td::mutex_startup2;
std::condition_variable Td::cv_startup2;
bool Td::startup2 = false;
bool Td::chat_list_done = false;
std::string Td::chatName;
std::string Td::SEkeyID;
bool Td::useSEcube = false;

std::int32_t Td::my_telegram_id;

std::unordered_map <long, SEchat> Td::chat_list; // map of Telegram chats (chat ID, SEchat object)
bool Td::chat_list_received = false; // this tells if the chat list have been received or not (used on startup)

std::vector<td::tl::unique_ptr<td_api::message>> Td::messages;
bool Td::chat_closed = false;
bool Td::chat_closed_error = false;
bool Td::chat_opened = false;
bool Td::chat_opened_error = false;
bool Td::messages_viewed = false;
bool Td::messages_viewed_error = false;

std::vector<td::tl::unique_ptr<td_api::message>> Td::previous_messages;

td::tl::unique_ptr<td_api::file> Td::file;

td::tl::unique_ptr<td_api::InputMessageContent> Td::inputMessageContent;

std::vector<td::tl::unique_ptr<td_api::message>> Td::newMessages;

std::map<std::int32_t, td_api::object_ptr<td_api::user>> Td::users_;

Td::Td() {
    td::Client::execute({0, td_api::make_object<td_api::setLogVerbosityLevel>(1)});
    client_ = std::make_unique<td::Client>();
}

void Td::loop() {
    while (true) {
        if (need_restart_) {
            restart();
        } else if (!are_authorized_) {
            process_response(client_->receive(10));
        } else {
            {
                std::lock_guard<std::mutex> lk(Td::mutex_ready);
                Td::thread_ready = true;
            }
            Td::cv_ready.notify_one();

            std::unique_lock<std::mutex> u_lk(Td::mutex_);
            Td::cv_.wait(u_lk, [this]{return Td::action_sent;});
            action_sent = false;

            if (action == "quit") {
                return;
            }
            else if (action == "update") {
                while (true) {
                    auto response = client_->receive(0);
                    if (response.object) {
                        process_response(std::move(response));
                    } else {
                        ok = true;
                        break;
                    }
                }
                u_lk.unlock();
                cv_.notify_one();
            }
            else if (action == "startup") {
                // get info about current Telegram user (the current user is the person who accessed his Telegram account with this app)
                send_query(td_api::make_object<td_api::getMe>(),[this](Object object) {
                               if (object->get_id() == td_api::error::ID) {
                                   printerr("Error" + to_string(object));
                                   error = true;
                                   return;
                               }
                               auto me = td::move_tl_object_as<td_api::user>(object);
                               my_telegram_id = me->id_;
                               ok = true;
                           });
                waitForServerResponse();
                if (error) { // close app
                    u_lk.unlock();
                    cv_.notify_one();
                    return;
                }
                // get info about chats of current Telegram user
                ok = false; error = false;
                send_query(td_api::make_object<td_api::getChats>(nullptr,
                            std::numeric_limits<std::int64_t>::max(), 0, DEFAULT_GETCHATS),
                           [this](Object object) {
                               if (object->get_id() == td_api::error::ID) {
                                   printerr("Error " + to_string(object));
                                   error = true;
                                   return;
                               }
                               chat_list_received = true; // chat list is immutable from this moment on (until next execution)
                               ok = true;
                           });
                waitForServerResponse(); // wait until Telegram data are processed
                std::unordered_map <long, SEchat> chat_list_temp;
                for(auto& it : chat_list){
                    long chatID = it.first;
                    SEchat chatInfo = it.second;
                    if(chatInfo.is_SEcube_chat == -1){
                        std::string request_str = "is_secube_chat_" + std::to_string(it.second.chat_id);
                        send(SEcube_socket, request_str.c_str(), request_str.length(), 0);
                        char server_res[2] = {0};
                        int res = read(SEcube_socket, server_res, 1);
                        if (res == 1) {
                            server_res[res] = '\0';
                            std::string resp(server_res);
                            if (resp == "1") { // this chat is protected by the SEcube
                                chatInfo.is_SEcube_chat = 1;
                            }
                            if (resp == "0") { // the chat can be used without encryption
                                chatInfo.is_SEcube_chat = 0;
                            }
                            if (resp == "2") { // chat not found on the SEcube, ask to the user
                                { /* Notify GUI to exit from wait on intermediate results and interact with the user */
                                    std::lock_guard<std::mutex> startup_lk(Td::mutex_startup);
                                    Td::chatName = chatInfo.chat_name; // tell GUI who is the person/group of this chat
                                    Td::startup = true;
                                    Td::chat_list_done = false;
                                }
                                Td::cv_startup.notify_one();
                                {   // wait for results
                                    std::unique_lock<std::mutex> startup_lk(Td::mutex_startup2);
                                    Td::cv_startup2.wait(startup_lk, []{return Td::startup2;});
                                    Td::startup2 = false;
                                }
                                if(Td::useSEcube){ // user requested to encrypt this chat
                                    if(Td::SEkeyID.empty()){
                                        printerr("User sent empty SEkey ID to encrypt chat with " + chatInfo.chat_name);
                                        chatInfo.is_SEcube_chat = -1;
                                        request_str = "";
                                    } else {
                                        chatInfo.is_SEcube_chat = 1; // set flag to 1
                                        chatInfo.SEkeyID = Td::SEkeyID; // copy ID for SEkey
                                        request_str = "add_enc_table_" + std::to_string(chatInfo.chat_id) + "_" + chatInfo.SEkeyID;
                                    }
                                } else {
                                    chatInfo.is_SEcube_chat = 0; // user did not request to encrypt this chat
                                    request_str = "add_not_enc_table_" + std::to_string(chatInfo.chat_id);
                                }
                                if(!request_str.empty()){
                                    send(SEcube_socket, request_str.c_str(), request_str.length(), 0);
                                    memset(server_res, '\0', 2);
                                    if (read(SEcube_socket, server_res, 1) == 1) {
                                        if(server_res[0] == '0'){ // OK
                                            // nothing to do
                                        } else { // error
                                            if(Td::useSEcube){ // handle error only if encrypted chat, non-encrypted chat do not require specific error handling
                                                printerr("Error adding encrypted chat with " + chatInfo.chat_name + " to Telegram database on SEcube");
                                                chatInfo.is_SEcube_chat = -1; // this will invalidate current chat entry
                                            }
                                        }
                                    } else {
                                        if(Td::useSEcube){ // handle error only if encrypted chat, non-encrypted chat do not require specific error handling
                                            printerr("Error adding encrypted chat with " + chatInfo.chat_name + " to Telegram database on SEcube");
                                            chatInfo.is_SEcube_chat = -1; // this will invalidate current chat entry
                                        }
                                    }
                                }
                            }
                            if (resp == "3"){
                                printerr("SEcube server did not respond with a valid answer about is_SEcube_chat for chat with " + chatInfo.chat_name);
                                chatInfo.is_SEcube_chat = -1; // Error on SEcube server, simply add chat to list with SEcube flag set to -1
                            }
                        } else {
                            printerr("SEcube server did not respond with a valid answer about is_SEcube_chat for chat with " + chatInfo.chat_name);
                            chatInfo.is_SEcube_chat = -1; // Error on SEcube server, simply add chat to list with SEcube flag set to -1
                        }
                    } else {
                        if (it.second.is_SEcube_chat != 0 && it.second.is_SEcube_chat != 1) { // invalid value
                            printerr("is_SEcube_chat value for chat with " + chatInfo.chat_name + " is neither -1, 0 or 1");
                            chatInfo.is_SEcube_chat = -1;
                        }
                        // here chatInfo.is_SEcube_chat is 0, 1 or -1
                    }
                    chat_list_temp.insert(std::pair<long, SEchat>(chatID, chatInfo)); // add to temp list
                }
                chat_list.swap(chat_list_temp); // swap temp list with original list
                { /* Notify GUI to exit from wait on intermediate results */
                    std::lock_guard<std::mutex> startup_lk(Td::mutex_startup);
                    Td::startup = true;
                    Td::chat_list_done = true;
                }
                Td::cv_startup.notify_one();
                // done, reset flag for condition variable and restart from the beginning
                action_sent = false; // this is redundant, action_sent is set to false just after the wait around line 120
                u_lk.unlock();
                cv_.notify_one(); // wake up GUI thread that is waiting for the response to be ready
                if (error) {
                    return;
                }
            }
            else if (action == "chat_messages") {
                if (closed_chat_id != 0) { // close previously selected chat (if any)
                    send_query(td_api::make_object<td_api::closeChat>(closed_chat_id),
                               [this](Object object) {
                                   if (object->get_id() == td_api::error::ID) {
                                       printerr("Error " + to_string(object));
                                       chat_closed_error = true;
                                       return;
                                   }
                                   chat_closed = true;
                               });
                    while (!chat_closed && !chat_closed_error) {
                        auto response = Td::client_->receive(0);
                        print_response(response);
                        if (response.object) {
                            Td::process_response(std::move(response));
                        }
                    }
                }
                // open new chat
                auto chat_id = selected_chat.chat_id;
                send_query(td_api::make_object<td_api::openChat>(chat_id),
                           [this](Object object) {
                           if (object->get_id() == td_api::error::ID) {
                               printerr("Error " + to_string(object));
                               chat_opened_error = true;
                               return;
                           }
                           chat_opened = true;
                       });
                while (!chat_opened && !chat_opened_error) {
                    auto response = Td::client_->receive(0);
                    print_response(response);
                    if (response.object) {
                        Td::process_response(std::move(response));
                    }
                }
                // get history of new chat
                send_query(td_api::make_object<td_api::getChatHistory>(
                        chat_id, 0, 0, 1, false),
                           [this, chat_id](Object object) {
                               if (object->get_id() == td_api::error::ID) {
                                   printerr("Error " + to_string(object));
                                   error = true;
                                   return;
                               }
                               // convert generic object to structured data
                               auto messages = td::move_tl_object_as<td_api::messages>(object);
                               auto last_message_id = messages->messages_[0].get()->id_;
                               Td::messages.insert(Td::messages.begin(), std::move(messages->messages_[0]));
                               send_query(td_api::make_object<td_api::getChatHistory>(
                                       chat_id, last_message_id, 0, 10, false),
                                          [this, chat_id](Object object) {
                                              if (object->get_id() == td_api::error::ID) {
                                                  printerr("Error " + to_string(object));
                                                  error = true;
                                                  return;
                                              }
                                              // convert generic object to structured data
                                              auto messages = td::move_tl_object_as<td_api::messages>(object);
                                              Td::messages.insert(Td::messages.begin(), std::make_move_iterator(messages->messages_.begin()), std::make_move_iterator(messages->messages_.end()));
                                              ok = true;
                                          });
                               waitForServerResponse();
                           });
                waitForServerResponse();
                // mark messages as read
                std::vector<std::int64_t> message_ids;
                for (auto & message : Td::messages) {
                    message_ids.insert(message_ids.begin(),message->id_);
                }
                send_query(td_api::make_object<td_api::viewMessages>(
                        chat_id, std::move(message_ids), true),
                           [this](Object object) {
                               if (object->get_id() == td_api::error::ID) {
                                   printerr("Error " + to_string(object));
                                   messages_viewed_error = true;
                                   return;
                               }
                               messages_viewed = true;
                           });
                while (!messages_viewed && !messages_viewed_error) {
                    auto response = Td::client_->receive(0);
                    print_response(response);
                    if (response.object) {
                        Td::process_response(std::move(response));
                    }
                }
                // wake up GUI thread that is waiting for the response
                u_lk.unlock();
                cv_.notify_one();
            }
            else if (action == "previous_messages") {
                auto chat_id = selected_chat.chat_id;
                std::int64_t from_message_id = 0;
                // if messages vector is not empty, find id of oldest message
                if(!Td::messages.empty()){
                    from_message_id = Td::messages.front()->id_;
                    auto oldest = Td::messages.front()->date_;
                    for(auto& msg_ : Td::messages){
                        if(msg_->date_ < oldest){ // there is an older message
                            oldest = msg_->date_;
                            from_message_id = msg_->id_;
                        }
                    }
                }
                // get old messages
                send_query(td_api::make_object<td_api::getChatHistory>(
                        chat_id, from_message_id, 0, 10, false),
                           [this, chat_id](Object object) {
                               if (object->get_id() == td_api::error::ID) {
                                   printerr("Error " + to_string(object));
                                   error = true;
                                   return;
                               }
                               auto messages = td::move_tl_object_as<td_api::messages>(object);
                               if(!messages->messages_.empty()){
                                   Td::previous_messages.insert(Td::previous_messages.begin(),
                                                                std::make_move_iterator(messages->messages_.begin()),
                                                                std::make_move_iterator(messages->messages_.end()));
                               }
                               ok = true;
                           });
                waitForServerResponse();
                // mark messages as read
                std::vector<std::int64_t> message_ids;
                for (auto & message : Td::previous_messages) {
                    message_ids.insert(message_ids.begin(),message->id_);
                }
                send_query(td_api::make_object<td_api::viewMessages>(
                        chat_id, std::move(message_ids), true),
                           [this](Object object) {
                               if (object->get_id() == td_api::error::ID) {
                                   printerr("Error " + to_string(object));
                                   messages_viewed_error = true;
                                   return;
                               }
                               messages_viewed = true;
                           });
                while (!messages_viewed && !messages_viewed_error) {
                    auto response = Td::client_->receive(0);
                    print_response(response);
                    if (response.object) {
                        Td::process_response(std::move(response));
                    }
                }
                // wake up GUI thread
                u_lk.unlock();
                cv_.notify_one();
            }
            else if (action == "send_message") {
                auto send_message = td_api::make_object<td_api::sendMessage>();
                send_message->chat_id_ = selected_chat.chat_id;
                send_message->input_message_content_ = std::move(Td::inputMessageContent);
                send_query(std::move(send_message), [this](Object object) {
                    if (object->get_id() == td_api::error::ID) {
                        error = true;
                        return;
                    }
                    auto sent_message = td::move_tl_object_as<td_api::message>(object);
                    Td::messages.insert(Td::messages.end(), std::move(sent_message));
                    ok = true;
                });
                waitForServerResponse();
                // wake up GUI thread
                u_lk.unlock();
                cv_.notify_one();
            }
            else if (action.rfind("file_", 0) == 0) {
                std::string file_id_str = action.substr(5);
                std::int32_t file_id = stoi(file_id_str, nullptr, 10);
                // receives "updateFile" updates until the file is not
                // completely downloaded; finally it executes the handler
                send_query(td_api::make_object<td_api::downloadFile>(
                        file_id, 32, 0, 0, true),
                           [this](Object object) {
                               if (object->get_id() == td_api::error::ID) {
                                   std::string tmps = "Error in downloading file (";
                                   tmps.append(std::to_string(object->get_id()));
                                   tmps.append(")");
                                   printerr(tmps);
                                   printerr(to_string(object));
                                   error = true;
                                   return;
                               }
                               if (!ok) {
                                   auto file = td::move_tl_object_as<td_api::file>(object);
                                   Td::file = std::move(file);
                                   ok = true;
                               }
                           });

                waitForServerResponse();
                u_lk.unlock();
                cv_.notify_one();
            }
        }
    }
}

void Td::restart() {
    client_.reset();
    *this = Td();
}

void Td::send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler) {
    auto query_id = next_query_id();
    if (handler) {
        handlers_.emplace(query_id, std::move(handler));
    }
    client_->send({query_id, std::move(f)});
}

void Td::process_response(td::Client::Response response) {
    if (!response.object) {
        return;
    }
    if (response.id == 0) {
        return process_update(std::move(response.object));
    }
    auto it = handlers_.find(response.id);
    if (it != handlers_.end()) {
        it->second(std::move(response.object));
    }
}

std::string Td::get_user_name(std::int32_t user_id) {
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return "unknown user";
    }
    return it->second->first_name_ + " " + it->second->last_name_;
}

void Td::process_update(td_api::object_ptr<td_api::Object> update) {
    td_api::downcast_call(
            *update, overloaded(
                    [this](td_api::updateAuthorizationState &update_authorization_state) {
                        authorization_state_ = std::move(update_authorization_state.authorization_state_);
                        on_authorization_state_update();
                    },
                    [this](td_api::updateNewChat &update_new_chat) {
                        if (!chat_list_received) {
                            auto chat_id = update_new_chat.chat_->id_;
                            if (chat_list.find(chat_id) == chat_list.end()) { // current chat ID is not stored in chat list yet
                                // new chat
                                SEchat chat = {
                                        update_new_chat.chat_->id_,
                                        update_new_chat.chat_->title_,
                                        update_new_chat.chat_->type_->get_id(),
                                        update_new_chat.chat_->unread_count_,
                                        -1,
                                        ""
                                };
                                chat_list.insert(std::pair<long, SEchat>(chat_id, chat)); // add current chat ID to chat list
                            }
                        }

                    },
                    [this](td_api::updateChatTitle &update_chat_title) {
                        chat_list.at(update_chat_title.chat_id_).chat_name = update_chat_title.title_;
                    },
                    [this](td_api::updateUser &update_user) {
                        auto user_id = update_user.user_->id_;
                        users_[user_id] = std::move(update_user.user_);
                    },
                    [this](td_api::updateNewMessage &update_new_message) {
                        auto chat_id = update_new_message.message_->chat_id_;
                        if (chat_id == selected_chat.chat_id) {
                            logmessage("Added new message to newMessages queue. Selected chat ID: " + std::to_string(selected_chat.chat_id) + ", new message chat ID: " + std::to_string(chat_id));
                            Td::newMessages.push_back(std::move(update_new_message.message_));
                        }
                    },
                    [this](td_api::updateChatReadInbox &update_chat_read_inbox) {
                        auto chat_id = update_chat_read_inbox.chat_id_;
                        auto chat = chat_list.find(chat_id);
                        if (chat != chat_list.end()) {
                            chat->second.unread_count = update_chat_read_inbox.unread_count_;
                            auto chat_button = chat_buttons.find(chat_id);
                            if(chat_button != chat_buttons.end()){
                                chat_button->second->setUnreadCounter(chat->second.unread_count);
                            }
                        }
                    },
                    [this](td_api::updateFile &update_file) {
                    },
                    [](auto &update) {
                    }));
}

auto Td::create_authentication_query_handler() {
    return [this, id = authentication_query_id_](Object object) {
        if (id == authentication_query_id_) {
            check_authentication_error(std::move(object));
        }
    };
}

void Td::on_authorization_state_update() {
    authentication_query_id_++;
    td_api::downcast_call(*authorization_state_,overloaded(
        [this](td_api::authorizationStateReady &) {
            are_authorized_ = true;
            {
                std::unique_lock<std::mutex> u_lk(Td::mutex_ready);
                Td::cv_ready.wait(u_lk, []{return Td::thread_ready;});
                Td::thread_ready = false;
            }
            {
                std::lock_guard<std::mutex> lk(mutex_);
                action = "login_done";
                action_sent = true;
            }
            cv_.notify_one();
            {   // Block Telegram thread until GUI sends notify on cv_ready
                std::unique_lock<std::mutex> u_lk(Td::mutex_ready); // thread_ready should be false when entering this wait()
                Td::cv_ready.wait(u_lk, []{return Td::thread_ready;});
                Td::thread_ready = false; // thread ready is set to false on exit (because it must be consistent with its previous value)
            }
        },
        [this](td_api::authorizationStateLoggingOut &) {
            are_authorized_ = false;
            logmessage("Telegram thread: logging out");
        },
        [this](td_api::authorizationStateClosing &) { logmessage("Telegram thread: closing"); },
        [this](td_api::authorizationStateClosed &) {
            are_authorized_ = false;
            need_restart_ = true;
            logmessage("Telegram thread terminated");
        },
        [this](td_api::authorizationStateWaitCode &) {
            //Telegram authentication code
            {
                std::unique_lock<std::mutex> u_lk(Td::mutex_ready);
                Td::cv_ready.wait(u_lk, []{return Td::thread_ready;});
                Td::thread_ready = false;
            }
            {
                std::lock_guard<std::mutex> lk(mutex_);
                action = "code";
                action_sent = true;
            }
            cv_.notify_one();
            {
                std::unique_lock<std::mutex> u_lk_login(mutex_);
                cv_.wait(u_lk_login, []{return input_wrote; });
                input_wrote = false;
            }
            send_query(td_api::make_object<td_api::checkAuthenticationCode>(code),
                       create_authentication_query_handler());
        },
        [this](td_api::authorizationStateWaitRegistration &) {
//            std::string first_name;
//            std::string last_name;
//            std::cout << "Enter your first name: " << std::flush;
//            std::cin >> first_name;
//            std::cout << "Enter your last name: " << std::flush;
//            std::cin >> last_name;
//            send_query(td_api::make_object<td_api::registerUser>(first_name, last_name),
//                       create_authentication_query_handler());
        },
        [this](td_api::authorizationStateWaitPassword &) {
//            std::cout << "Enter authentication password: " << std::flush;
//            std::string password;
//            std::cin >> password;
//            send_query(td_api::make_object<td_api::checkAuthenticationPassword>(password),
//                       create_authentication_query_handler());
        },
        [this](td_api::authorizationStateWaitOtherDeviceConfirmation &state) {
//            std::cout << "Confirm this login link on another device: " << state.link_ << std::endl;
        },
        [this](td_api::authorizationStateWaitPhoneNumber &) {
            //Enter phone number (with prefix)
            {
                std::unique_lock<std::mutex> u_lk(Td::mutex_ready);
                Td::cv_ready.wait(u_lk, []{return Td::thread_ready;});
                Td::thread_ready = false;
            }
            {
                std::lock_guard<std::mutex> lk(mutex_);
                action = "phone";
                action_sent = true;
            }
            cv_.notify_one();
            {
                std::unique_lock<std::mutex> u_lk_login(mutex_);
                cv_.wait(u_lk_login, []{ return input_wrote; });
                input_wrote = false;
            }
            send_query(td_api::make_object<td_api::setAuthenticationPhoneNumber>(phone, nullptr),
                       create_authentication_query_handler());
        },
        [this](td_api::authorizationStateWaitEncryptionKey &) {
            //TDLib needs an encryption key to decrypt the local TDLib database
            {
                std::unique_lock<std::mutex> u_lk(Td::mutex_ready);
                Td::cv_ready.wait(u_lk, []{return Td::thread_ready;});
                Td::thread_ready = false;
            }
            {
                std::lock_guard<std::mutex> lk(mutex_);
                action = "key";
                action_sent = true;
            }
            cv_.notify_one();
            {
                std::unique_lock<std::mutex> u_lk_login(mutex_);
                cv_.wait(u_lk_login, []{return input_wrote; });
                input_wrote = false;
            }
            if (key == "DESTROY") {
                send_query(td_api::make_object<td_api::destroy>(),
                           create_authentication_query_handler());
            } else {
                send_query(td_api::make_object<td_api::checkDatabaseEncryptionKey>(std::move(key)),
                           create_authentication_query_handler());
            }

        },
        [this](td_api::authorizationStateWaitTdlibParameters &) {
            auto parameters = td_api::make_object<td_api::tdlibParameters>();
            parameters->database_directory_ = "tdlib";
            parameters->use_message_database_ = true;
            parameters->use_secret_chats_ = true;
            parameters->api_id_ = 94575;
            parameters->api_hash_ = "a3406de8d171bb422bb6ddf3bbd800e2";
            parameters->system_language_code_ = "en";
            parameters->device_model_ = "Desktop";
            parameters->system_version_ = "Unknown";
            parameters->application_version_ = "1.0";
            parameters->enable_storage_optimizer_ = true;
            send_query(td_api::make_object<td_api::setTdlibParameters>(std::move(parameters)),
                       create_authentication_query_handler());
        }));
}

void Td::check_authentication_error(Object object) {
    if (object->get_id() == td_api::error::ID) {
        auto error_ = td::move_tl_object_as<td_api::error>(object);
        printerr("Error: " + to_string(error_));
        on_authorization_state_update();
    }
}

std::uint64_t Td::next_query_id() {
    return ++current_query_id_;
}

void Td::waitForServerResponse() {
    while (!ok && !error) {
        auto response = Td::client_->receive(0);
        if (response.object) {
            print_response(response);
            Td::process_response(std::move(response));
        }
    }
}

void Td::print_response(td::Client::Response& response) {
    logmessage("==============");
    logmessage(std::to_string(response.id) + ":");
    logmessage("--------------");
    logmessage(to_string(response.object));
    logmessage("==============");
}
