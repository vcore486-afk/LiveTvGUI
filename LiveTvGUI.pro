QT       += core gui widgets webenginewidgets webenginecore

CONFIG   += c++17

TARGET   = LiveTvGUI-Qt6
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    playerwindow.cpp \
    m3u8interceptor.cpp        # ← добавили!

HEADERS += \
    mainwindow.h \
    playerwindow.h \
    m3u8interceptor.h          # ← и заголовок тоже

FORMS += \
    mainwindow.ui \
    playerwindow.ui

RESOURCES += resources.qrc