include(../defaults.pri)

TARGET = kMC-tests

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lunittest++ -L$$TOP_OUT_PWD/src/libs -lkMC

SOURCES = testmain.cpp