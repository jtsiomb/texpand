QT += core gui widgets

CONFIG += debug c++11

TARGET = texpand-gui
TEMPLATE = app

# GUI
SOURCES += src/main.cc src/mainwin.cc \
    src/imageview.cc
HEADERS += src/mainwin.h \
    src/imageview.h
FORMS += ui/mainwin.ui

# backend
QMAKE_CFLAGS += -fopenmp
SOURCES += ../src/genmask.c ../src/expand.c
LIBS += -lassimp -limago -lX11 -lgomp

INCLUDEPATH += src ../src

isEmpty(PREFIX) {
	PREFIX = /usr/local
}

target.path = $$PREFIX/bin
target.files += $$TARGET
INSTALLS += target
