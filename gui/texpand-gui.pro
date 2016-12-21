QT += core gui widgets

CONFIG += debug c++11

TARGET = texpand-gui
TEMPLATE = app

# GUI
SOURCES += src/main.cc src/mainwin.cc
HEADERS += src/mainwin.h
FORMS += ui/mainwin.ui

# backend
QMAKE_CFLAGS += -fopenmp
SOURCES += ../src/genmask.c ../src/expand.c
LIBS += -lassimp -limago -lX11 -lgomp

INCLUDEPATH += ../src

isEmpty(PREFIX) {
	PREFIX = /usr/local
}

target.path = $$PREFIX/bin
target.files += $$TARGET
INSTALLS += target
