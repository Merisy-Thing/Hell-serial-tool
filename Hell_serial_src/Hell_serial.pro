#-------------------------------------------------
#
# Project created by QtCreator 2011-08-19T16:51:25
#
#-------------------------------------------------

QT       += core gui

TARGET = Hell_serial
TEMPLATE = app

INCLUDEPATH += qextserialport


SOURCES += main.cpp\
        hell_serial.cpp \
    qextserialport/win_qextserialport.cpp \
    qextserialport/qextserialport.cpp \
    qextserialport/qextserialenumerator.cpp \
    qextserialport/qextserialbase.cpp \
    receive_thread.cpp

HEADERS  += hell_serial.h \
    qextserialport/win_qextserialport.h \
    qextserialport/qextserialport.h \
    qextserialport/qextserialenumerator.h \
    qextserialport/qextserialbase.h \
    receive_thread.h

FORMS    += hell_serial.ui

RC_FILE = icon.rc

RESOURCES += \
    rc.qrc
