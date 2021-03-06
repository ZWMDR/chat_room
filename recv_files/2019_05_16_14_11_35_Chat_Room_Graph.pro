#-------------------------------------------------
#
# Project created by QtCreator 2019-04-29T19:21:43
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Chat_Room_Graph
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        log_in.cpp \
    chat_window.cpp \
    emoji.cpp

HEADERS += \
        log_in.h \
    chat_window.h \
    ip_info.h \
    emoji.h

FORMS += \
        log_in.ui \
    chat_window.ui \
    emoji.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RC_ICONS=Logo/1.ico
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         SYS_SIGNAL_ONLINE_COUNT:002 2019/05/16 14:11:35
ZWMDR:
	                                                                                                                                                                                                                                                                                                                                    