#-------------------------------------------------
#
# Project created by QtCreator 2016-03-15T08:29:33
#
#-------------------------------------------------

QT       += core gui opengl printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = brainstem
TEMPLATE = app

CONFIG -= debug_and_release
CONFIG += warn_off

QMAKE_CXXFLAGS += -Wall -Wno-strict-aliasing -std=c++14

win32 {
   #QMAKE_CXXFLAGS += -I/opt/mxe/usr/include 
   QMAKE_CXXFLAGS += -std=c++17
   LIBS += -lwinpthread
   CONFIG -= debug
}

SOURCES += main.cpp\
           brainstem.cpp \
           brainstemgl.cpp \
           brainstemgl.glsl \
           brain_impl.cpp \
           all_structures.c \
           outlines.c \
           atlasnames.c \
           sphere.c \
           helpbox.cpp


HEADERS  += brainstem.h \
            brainstemgl.h \
            helpbox.h

FORMS    += brainstem.ui \
            helpbox.ui

RESOURCES += \
    brainstem.qrc

win32 {
OBJECTS_DIR = mswin
}

# If you run qtcreator qmake, it will clobber the autotools
# Makefile.  This causes qmake to output a Makefile
# with this name for development within the 
# qtcreator gui.
linux {
   MAKEFILE=Makefile.qt
   CONFIG += debug
}

win32 {
   MAKEFILE=Makefile_win.qt
}

