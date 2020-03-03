QT += core
QT -= gui

TARGET = stem2gl
CONFIG += console
CONFIG -= app_bundle

QMAKE_CXXFLAGS += -std=c++17 -ggdb3
QMAKE_CXXFLAGS_RELEASE -= -O2

TEMPLATE = app

SOURCES += \
    stem2gl.cpp

# If you run qtcreator, it will clobber the autotools
# Makefile.  This causes qmake to output a Makefile
# with this name for development within the 
# qtcreator gui.
MAKEFILE=Makefiles2gl.qt
