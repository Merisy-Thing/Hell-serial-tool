#-------------------------------------------------
#
# Project created by QtCreator 2011-08-19T16:51:25
#
#-------------------------------------------------

QT       += core gui network widgets serialport

TARGET = Hell_Serial
TEMPLATE = app

INCLUDEPATH += qextserialport


SOURCES += main.cpp\
        hell_serial.cpp

HEADERS  += hell_serial.h

FORMS    += hell_serial.ui

RC_FILE = icon.rc

RESOURCES += \
    rc.qrc
