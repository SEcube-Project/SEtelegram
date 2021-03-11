/**
  ******************************************************************************
  * File Name          : main.cpp
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

#include <iostream>
#include <QApplication>
#include "AccessWindow.h"
#include "global.h"

int main(int argc, char **argv) {
    logmessage("Starting SEcube thread...");
    std::string command = SEcubeServer_path + " " + std::to_string(port);
    secube_thread = std::thread([command]() {
        std::system(command.c_str());
    });

    QApplication a(argc, argv);
    QFile File(":/css/stylesheet.qss");
    File.open(QFile::ReadOnly);
    QString StyleSheet = QLatin1String(File.readAll());
    a.setStyleSheet(StyleSheet);
    File.close();

    AccessWindow aw;
    if(aw.socket_ok){
        aw.show();
        return a.exec();
    } else {
        return -1;
    }
}
