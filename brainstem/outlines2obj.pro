QT += core
QT -= gui

TARGET = outlines2obj
CONFIG += console
CONFIG -= app_bundle
CONFIG -= debug_and_release
CONFIG += warn_off

QMAKE_CXXFLAGS += -std=c++17 -ggdb3
QMAKE_CXXFLAGS_RELEASE -= -O2

TEMPLATE = app

SOURCES += \
    outlines2obj.cpp

# If you run qtcreator, it will clobber the autotools
# Makefile.  This causes qmake to output a Makefile
# with this name for development within the 
# qtcreator gui.
MAKEFILE=Makefileoutlines2obj.qt

