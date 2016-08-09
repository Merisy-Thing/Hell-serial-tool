#-------------------------------------------------
#
# Project created by QtCreator 2011-08-19T16:51:25
#
#-------------------------------------------------

QT       += core gui network widgets serialport

TARGET = Hell_Serial
TEMPLATE = app

INCLUDEPATH += qextserialport


SOURCES += main.cpp \
        hell_serial.cpp \
    qhexedit2/chunks.cpp \
    qhexedit2/commands.cpp \
    qhexedit2/qhexedit.cpp \
    custom_cmd_item.cpp \
    lua_plugin.cpp \
    lua_bind.cpp \
    lua_5.3.x/lapi.c \
    lua_5.3.x/lauxlib.c \
    lua_5.3.x/lbaselib.c \
    lua_5.3.x/lbitlib.c \
    lua_5.3.x/lcode.c \
    lua_5.3.x/lcorolib.c \
    lua_5.3.x/lctype.c \
    lua_5.3.x/ldblib.c \
    lua_5.3.x/ldebug.c \
    lua_5.3.x/ldo.c \
    lua_5.3.x/ldump.c \
    lua_5.3.x/lfunc.c \
    lua_5.3.x/lgc.c \
    lua_5.3.x/linit.c \
    lua_5.3.x/liolib.c \
    lua_5.3.x/llex.c \
    lua_5.3.x/lmathlib.c \
    lua_5.3.x/lmem.c \
    lua_5.3.x/loadlib.c \
    lua_5.3.x/lobject.c \
    lua_5.3.x/lopcodes.c \
    lua_5.3.x/loslib.c \
    lua_5.3.x/lparser.c \
    lua_5.3.x/lstate.c \
    lua_5.3.x/lstring.c \
    lua_5.3.x/lstrlib.c \
    lua_5.3.x/ltable.c \
    lua_5.3.x/ltablib.c \
    lua_5.3.x/ltm.c \
    lua_5.3.x/lundump.c \
    lua_5.3.x/lutf8lib.c \
    lua_5.3.x/lvm.c \
    lua_5.3.x/lzio.c \
    luahighlighter.cpp \
    code_editor.cpp

HEADERS  += hell_serial.h \
    qhexedit2/chunks.h \
    qhexedit2/commands.h \
    qhexedit2/qhexedit.h \
    custom_cmd_item.h \
    lua_plugin.h \
    lua_bind.h \
    lua_5.3.x/lapi.h \
    lua_5.3.x/lauxlib.h \
    lua_5.3.x/lcode.h \
    lua_5.3.x/lctype.h \
    lua_5.3.x/ldebug.h \
    lua_5.3.x/ldo.h \
    lua_5.3.x/lfunc.h \
    lua_5.3.x/lgc.h \
    lua_5.3.x/llex.h \
    lua_5.3.x/llimits.h \
    lua_5.3.x/lmem.h \
    lua_5.3.x/lobject.h \
    lua_5.3.x/lopcodes.h \
    lua_5.3.x/lparser.h \
    lua_5.3.x/lprefix.h \
    lua_5.3.x/lstate.h \
    lua_5.3.x/lstring.h \
    lua_5.3.x/ltable.h \
    lua_5.3.x/ltm.h \
    lua_5.3.x/lua.h \
    lua_5.3.x/lua.hpp \
    lua_5.3.x/luaconf.h \
    lua_5.3.x/lualib.h \
    lua_5.3.x/lundump.h \
    lua_5.3.x/lvm.h \
    lua_5.3.x/lzio.h \
    luahighlighter.h \
    code_editor.h

FORMS    += hell_serial.ui \
    lua_plugin.ui

RC_FILE = icon.rc

RESOURCES += \
    rc.qrc
