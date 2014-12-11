#-------------------------------------------------
#
# Project created by QtCreator 2014-11-27T17:13:23
#
#-------------------------------------------------

QT       += sql core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Sukaldu
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    connection.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    data/database_definition.sql

data.path    = $$OUT_PWD/data
data.files   += \
    data/database_definition.sql
INSTALLS       += data