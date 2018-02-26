#-------------------------------------------------
#
# Project created by QtCreator 2017-12-22T00:45:05
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RecorderKinect
TEMPLATE = app

CONFIG += c++11

QMAKE_CXXFLAGS = -fpermissive -O3 -msse4.2

SOURCES += \
    viewer.cpp \
    flextGL.cpp \
    kinectonestream.cpp

HEADERS  += \
    viewer.h \
    flextGL.h \
    main.old


INCLUDEPATH += /usr/local/include/ \
INCLUDEPATH += /usr/local/include/opencv \


LIBS += -L/usr/local/lib \
        -lopencv_core \
        -lopencv_imgproc \
        -lopencv_features2d \
        -lopencv_highgui \
        -lopencv_objdetect \
        -lopencv_imgproc \
        -lopencv_calib3d \
        -lopencv_photo \
        -lfreenect2 \
        -lglfw \
