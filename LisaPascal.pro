QT       += core
QT       -= gui

TARGET = LisaPascal
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += ..
DEFINES += _DEBUG

SOURCES += main.cpp \
    LisaLexer.cpp \
    LisaParser.cpp \
    LisaSynTree.cpp \
    LisaTokenType.cpp \
    Converter.cpp \
    FileSystem.cpp \
    PpLexer.cpp

HEADERS += \
    LisaLexer.h \
    LisaParser.h \
    LisaSynTree.h \
    LisaToken.h \
    LisaTokenType.h \
    Converter.h \
    FileSystem.h \
    PpLexer.h
