TEMPLATE    = app
CONFIG        += qt warn_on
HEADERS        = screen.h \
                 frontpanel.h \
                 comborange.h \
                 oscilloscope.h \
                 acquisition.h \
                 acquisition2000.h \
                 acquisition2000a.h \
                 acquisition3000.h \
                 mainwindow.h \
                 search-for-acquisition-device-worker.h
SOURCES        = screen.cpp \
                 frontpanel.cpp \
                 main.cpp \
                 comborange.cpp \
                 acquisition.cpp \
                 acquisition2000.cpp \
                 acquisition2000a.cpp \
                 acquisition3000.cpp \
                 mainwindow.cpp \
                 search-for-acquisition-device-worker.cpp
TARGET        = QPicoscope
QTDIR_build:REQUIRES="contains(QT_CONFIG, full-config)"
unix:LIBS += -lm -lps2000 -lps3000

# install
target.path = ./
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS qpicoscope.pro
sources.path = ./

INSTALLS += target sources


#unix {
#    QWT_INSTALL_PREFIX    = /usr/local/qwt-$$QWT_VERSION
#}

#win32 {
#    QWT_INSTALL_PREFIX    = C:/Qwt-$$QWT_VERSION
#}

#QWT_CONFIG       += QwtPlot

CONFIG += qwt
INCLUDEPATH += /usr/include/qwt-qt4
LIBS      += -lqwt-qt4

