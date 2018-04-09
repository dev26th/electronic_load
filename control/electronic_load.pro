#-------------------------------------------------
#
# Project created by QtCreator 2017-10-29T23:16:29
#
#-------------------------------------------------

QMAKE_CXXFLAGS += -std=c++11
QT       += core gui serialport

QT += widgets

TARGET = electronic_load
TEMPLATE = app

SOURCES += main.cpp\
    mainwindow.cpp \
    comm.cpp \
    utils.cpp \
    samplestorage.cpp \
    curvedata.cpp \
    tablemodel.cpp \
    logtable.cpp \
    aboutdialog.cpp \
    decoder.cpp \
    configdialog.cpp \
    flasherworker.cpp \
    flasher.cpp \
    flashprogressdialog.cpp \
    crc.cpp

HEADERS  += mainwindow.h \
    decoder.h \
    comm.h \
    utils.h \
    samplestorage.h \
    curvedata.h \
    tablemodel.h \
    sample.h \
    logtable.h \
    settings.h \
    aboutdialog.h \
    stdio_fix.h \
    configdialog.h \
    flasherworker.h \
    flasher.h \
    flashprogressdialog.h \
    crc.h

FORMS    += mainwindow.ui \
    aboutdialog.ui \
    configdialog.ui \
    flashprogressdialog.ui

RC_ICONS = app.ico

DISTFILES +=

RESOURCES += \
    common.qrc

unix {
    CONFIG   += qwt-qt5
    INCLUDEPATH += /usr/include/qwt
    LIBS += -lqwt-qt5
}

win32 {
    include (C:/qwt-6.1.3/features/qwt.prf)
    CONFIG += qwt
}
