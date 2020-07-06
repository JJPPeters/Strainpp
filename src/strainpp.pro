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
    Strain

win32: INCLUDEPATH += D:\Programming\Cpp\qcustomplot \
    D:\Programming\Cpp\eigen3.28

unix:!macx: INCLUDEPATH += /home/jon/Programming/qcustomplot \
    /home/jon/Programming/eigen_3.2.9

SOURCES += main.cpp\
    mainwindow.cpp \
    Strain/phase.cpp \
    Strain/gpa.cpp \
    Utils/exceptions.cpp

win32: SOURCES += D:\Programming\Cpp\qcustomplot\qcustomplot.cpp

unix:!macx: SOURCES += /home/jon/Programming/qcustomplot/qcustomplot.cpp

HEADERS  += mainwindow.h \
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

win32: HEADERS += D:\Programming\Cpp\qcustomplot\qcustomplot.h

unix:!macx: HEADERS += /home/jon/Programming/qcustomplot/qcustomplot.h

FORMS    += mainwindow.ui

RESOURCES += \
    axesresource.qrc

win32: LIBS += -L$$PWD/../../../Programming/Cpp/FFTW3/ -llibfftw3-3

INCLUDEPATH += $$PWD/../../../Programming/Cpp/FFTW3
DEPENDPATH += $$PWD/../../../Programming/Cpp/FFTW3

win32: LIBS += -L$$PWD/../../../Programming/Cpp/msys64/usr/local/lib/ -ltiff

INCLUDEPATH += $$PWD/../../../Programming/Cpp/msys64/usr/local/include
DEPENDPATH += $$PWD/../../../Programming/Cpp/msys64/usr/local/include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../../Programming/Cpp/msys64/usr/local/lib/tiff.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../../../Programming/Cpp/msys64/usr/local/lib/libtiff.a

unix:!macx: LIBS += -L/usr/lib/x86_64-linux-gnu/ -lfftw3

INCLUDEPATH += /usr/include
DEPENDPATH += /usr/include

unix:!macx: LIBS += -L/usr/lib/x86_64-linux-gnu/ -ltiff

INCLUDEPATH += /usr/include/x86_64-linux-gnu
DEPENDPATH += /usr/include/x86_64-linux-gnu
