TEMPLATE = app
CONFIG += console
CONFIG -= qt

INCLUDEPATH += /usr/local/include

LIBS += -L/usr/local/lib -lboost_program_options -lboost_thread\
        -L/usr/lib -lDeckLinkAPI -lDeckLinkPreviewAPI

SOURCES += main.cpp \
    decklinkcapturedelegate.cpp \
    capturewatcher.cpp

HEADERS += \
    decklinkcapturedelegate.h \
    capturewatcher.h

