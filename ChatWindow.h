/**
  ******************************************************************************
  * File Name          : ChatWindow.h
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

#ifndef SETELEGRAM_CHATWINDOW_H
#define SETELEGRAM_CHATWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include "Td.h"
#include "Frames/ImageMessageFrame.h"
#include "Frames/ChatButtonFrame.h"

namespace Ui {
    class ChatWindow;
}

class ChatWindow : public QMainWindow {
Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = nullptr);
    ~ChatWindow() override;
    void closeEvent(QCloseEvent *event) override;
    //bool eventFilter(QObject *obj, QEvent *event) override;
    template<class Duration> bool wait_for(Duration duration);

public slots:
    void chatButtonClicked(long chat_id);
    void documentButtonClicked(std::int32_t file_id, const std::string& file_name);
    void attachmentButtonClicked();
    void sendButtonClicked();
    void newMessagesUpdate();
    void setScrollbarPosition();
    void getPreviousMessages();

signals:
    void checkNewMessages();

private:
    Ui::ChatWindow *ui;
    QVBoxLayout* messagesLayout;
    QWidget* messages_container;
    QLabel* selezionaChatLabel;
    std::string attachmentPath;
    std::vector<ImageMessageFrame*> image_messages;

    void sendMessage();
    void showMessage(td::tl::unique_ptr<td::td_api::message>& message, bool put_at_top);
    void showImages();
    void waitTelegram();
    void requestTelegram(std::string& action_str);
    void requestTelegram_startup(std::string& action_str);
};


#endif //SETELEGRAM_CHATWINDOW_H
