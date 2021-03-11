cmake_minimum_required(VERSION 3.16)
project(SEtelegram)

set(Td_DIR "/home/matteo/Scaricati/td/tdlib/lib/cmake/Td")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Charts REQUIRED)

find_package(Td 1.6.0 REQUIRED)

#link_directories(sqlite)

#[[add_library( libsqlite3 STATIC IMPORTED )
set_property( TARGET libsqlite3 PROPERTY IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/libsqlite3.a )
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR}/sqlite )]]

list(APPEND LIBRARIES
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Charts
        #libsqlite3
        )
list(REMOVE_DUPLICATES LIBRARIES)

add_executable(
        SEtelegram
        main.cpp
        resources.qrc
        css/stylesheet.qss
        AccessWindow.cpp AccessWindow.h AccessWindow.ui
        ChatWindow.cpp ChatWindow.h ChatWindow.ui
        Dialogs/ErrorDialog.cpp Dialogs/ErrorDialog.h Dialogs/ErrorDialog.ui
        Frames/DocumentMessageFrame.cpp Frames/DocumentMessageFrame.h Frames/DocumentMessageFrame.ui

        #sekey/SEkey.cpp sekey/SEkey.h

        #sqlite/sqlite3.h

        #[[sources/L0/L0\ Base/L0_base.cpp sources/L0/L0\ Base/L0_base.h
        sources/L0/Communication\ API/communication_api.h
        sources/L0/L0.cpp sources/L0/L0.h
        sources/L0/L0_commodities.cpp
        sources/L0/L0_communication.cpp
        sources/L0/L0_logfile_managing.cpp
        sources/L0/L0_provision.cpp
        sources/L0/L0_enumerations.h
        sources/L0/L0_error_manager.h
        sources/L0/Provision\ API/provision_api.h

        sources/L1/Crypto\ Libraries/aes256.cpp sources/L1/Crypto\ Libraries/aes256.h
        sources/L1/Crypto\ Libraries/pbkdf2.cpp sources/L1/Crypto\ Libraries/pbkdf2.h
        sources/L1/Crypto\ Libraries/sha256.cpp sources/L1/Crypto\ Libraries/sha256.h
        sources/L1/L1\ Base/L1_base.cpp sources/L1/L1\ Base/L1_base.h
        sources/L1/L1.cpp sources/L1/L1.h
        sources/L1/L1_login_logout.cpp
        sources/L1/L1_security.cpp
        sources/L1/L1_enumerations.h
        sources/L1/L1_error_manager.h
        sources/L1/Login-Logout\ API/login_logout_api.h
        sources/L1/Security\ API/security_api.h
        sources/L1/Utility\ API/utility_api.h

        sources/LH/Commodities\ API/commodities_api.h

        selink/selink.cpp selink/selink.h
        selink/base64/base64.cpp selink/base64/base64.h
        selink/base64/ccommon.h
        selink/base64/cdecode.c selink/base64/cdecode.h
        selink/base64/cencode.c selink/base64/cencode.h

        sefile/environment.h
        sefile/SEcureDB.cpp sefile/SEcureDB.h
        sefile/SEfile.cpp sefile/SEfile.h
        sefile/SEfile_C_interface.h]]

        Td.cpp Td.h global.cpp global.h Frames/TextMessageFrame.cpp Frames/TextMessageFrame.h Dialogs/AttachmentDialog.cpp Dialogs/AttachmentDialog.h Frames/ImageMessageFrame.cpp Frames/ImageMessageFrame.h Frames/ChatButtonFrame.cpp Frames/ChatButtonFrame.h Dialogs/TelegramAccessDialog.cpp Dialogs/TelegramAccessDialog.h)



target_link_libraries(SEtelegram PRIVATE Td::TdStatic ${LIBRARIES})