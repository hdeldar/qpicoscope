include $(top_srcdir)/build-aux/autotroll.mk

# For a program:
bin_PROGRAMS = QPicoscope
QPicoscope_SOURCES = acquisition2000.cpp  acquisition3000.cpp  acquisition.cpp  comborange.cpp  frontpanel.cpp  main.cpp  mainwindow.cpp  screen.cpp \
                      drawdata.h  frontpanel.h  mainwindow.h  oscilloscope.h  screen.h \
                      drawdata.moc.cpp frontpanel.moc.cpp mainwindow.moc.cpp oscilloscope.moc.cpp screen.moc.cpp

QPicoscope_CXXFLAGS = $(QT_CXXFLAGS) $(AM_CXXFLAGS)
QPicoscope_CPPFLAGS = $(QT_CPPFLAGS) $(AM_CPPFLAGS)
QPicoscope_LDFLAGS  = $(QT_LDFLAGS) $(LDFLAGS)
QPicoscope_LDADD    = $(QT_LIBS) $(LDADD)

BUILT_SOURCES = drawdata.moc.cpp frontpanel.moc.cpp mainwindow.moc.cpp oscilloscope.moc.cpp screen.moc.cpp

