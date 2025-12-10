QT       += core gui widgets webenginewidgets webenginecore

CONFIG   += c++17

TARGET   = LiveTvGUI-Qt6
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    playerwindow.cpp \
    pythonmanager.cpp \
    m3u8interceptor.cpp        # ← добавили!

HEADERS += \
    mainwindow.h \
    playerwindow.h \
    pythonmanager.h \
    m3u8interceptor.h          # ← и заголовок тоже

FORMS += \
    mainwindow.ui \
    playerwindow.ui


INCLUDEPATH +=  /usr/local/include/python3.11    # замените x на номер вашей версии Python
LIBS += -L/usr/local/lib/python3.11-config/lib  # замените x на номер вашей версии Python
LIBS += -lpython3.11                     # замените x на номер вашей версии Python

RESOURCES += resources.qrc