TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += "../stdsocket"
VPATH += $${INCLUDEPATH}

SOURCES += \
	socket.cpp \
	client.cpp
	shifumiclient.cpp \


HEADERS += \
	socket.h
	shifumiclient.h

