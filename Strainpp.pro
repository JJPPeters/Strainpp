#-------------------------------------------------
#
# Project created by QtCreator 2015-05-11T23:01:26
#
#-------------------------------------------------

RC_FILE = Strainpp.rc
CONFIG += c++14

QT       += core gui svg
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS += -fopenmp
LIBS += -fopenmp

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = Strainpp
TEMPLATE = app

INCLUDEPATH += ReadDM \
    Plotting \
    Utils \
    Strain \
    D:\Programming\Cpp\boost_1_57_0 \
    D:\Programming\Cpp\qcustomplot \
    D:\Programming\Cpp\FFTW3 \
    D:\Programming\Cpp\eigen3.28 \

SOURCES += main.cpp\
        mainwindow.cpp \
    ../../../Programming/Cpp/qcustomplot/qcustomplot.cpp \
    Strain/phase.cpp \
    Strain/gpa.cpp \
    Utils/exceptions.cpp

HEADERS  += mainwindow.h \
    ../../../Programming/Cpp/qcustomplot/qcustomplot.h \
    Plotting/imageplot.h \
    ui_mainwindow.h \
    Utils/exceptions.h \
    Strain/phase.h \
    Strain/gpa.h \
    Utils/utils.h \
    ReadDM/dmutils.h \
    ReadDM/tagreader.h \
    ReadDM/streamreader.h \
    ReadDM/dmreader.h \
    Utils/coord.h \
    Plotting/colorbarplot.h

FORMS    += mainwindow.ui

win32: LIBS += -L$$PWD/../../../Programming/Cpp/FFTW3/ -llibfftw3-3

# INCLUDEPATH += $$PWD/../../../Programming/Cpp/FFTW3
DEPENDPATH += $$PWD/../../../Programming/Cpp/FFTW3

DISTFILES +=

win32: LIBS += -L$$PWD/../../../Programming/Cpp/msys64/usr/local/lib/ -ltiff

INCLUDEPATH += $$PWD/../../../Programming/Cpp/msys64/usr/local/include
DEPENDPATH += $$PWD/../../../Programming/Cpp/msys64/usr/local/include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../../Programming/Cpp/msys64/usr/local/lib/tiff.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../../../Programming/Cpp/msys64/usr/local/lib/libtiff.a

RESOURCES += \
    axesresource.qrc
