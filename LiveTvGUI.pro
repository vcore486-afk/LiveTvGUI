QT       += core gui widgets network webenginewidgets

CONFIG   += c++17 qt quick

TARGET   = LiveTvGUI-Qt6
TEMPLATE = app

# Это важно для Qt6!
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += webenginewidgets
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    playerwindow.cpp

HEADERS += \
    mainwindow.h \
    playerwindow.h

FORMS += \
    mainwindow.ui \
    playerwindow.ui

# Для совместимости с Qt5-кодом (если используешь QString::arg и т.п.)
QT += core5compat