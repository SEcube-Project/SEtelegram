/**
  ******************************************************************************
  * File Name          : AccessWindow.h
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

#ifndef SETELEGRAM_ACCESSWINDOW_H
#define SETELEGRAM_ACCESSWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include "ChatWindow.h"

namespace Ui {
    class AccessWindow;
}

class AccessWindow : public QMainWindow {

Q_OBJECT

public:
    explicit AccessWindow(QWidget *parent = nullptr);
    ~AccessWindow() override;
    void closeEvent(QCloseEvent *event) override;
    bool socket_ok = false;

private slots:
    void accessButtonClicked();

private:
    Ui::AccessWindow *ui;
    ChatWindow* cw;
    bool setup_ok = false;
};


#endif //SETELEGRAM_ACCESSWINDOW_H
