TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

CONFIG(release, debug|release) {
    #This is a release build
    DEFINES += NDEBUG
    QMAKE_CXXFLAGS += -s
} else {
    #This is a debug build
    DEFINES += DEBUG
    TARGET = $$join(TARGET,,,_d)
}

DESTDIR = $$PWD/bin

QMAKE_CXXFLAGS += -std=c++17 -Wno-unused-parameter -Wconversion -Wold-style-cast

INCLUDEPATH += $$PWD/include

LIBS += -L$$PWD/lib

unix:{
    LIBS += -lglfw -lGL -lGLEW
}
win32:{
    LIBS += -lglfw3dll -lopengl32 -lglew32.dll
    LIBS += -static-libgcc -static-libstdc++
    LIBS += -static -lpthread
}

SOURCES += \
    src/imagedata.cpp \
    src/main.cpp \
    src/window.cpp

HEADERS += \
    src/imagedata.h \
    src/window.h
