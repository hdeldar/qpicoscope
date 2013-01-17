include $(top_srcdir)/build-aux/autotroll.mk

# For a program:
bin_PROGRAMS = QPicoscope
QPicoscope_SOURCES = acquisition2000.cpp  acquisition3000.cpp  acquisition.cpp  comborange.cpp  frontpanel.cpp  main.cpp  mainwindow.cpp  screen.cpp \
                      comborange.h acquisition.h drawdata.h  frontpanel.h  mainwindow.h  oscilloscope.h  screen.h \
                      comborange.moc.cpp acquisition.moc.cpp drawdata.moc.cpp frontpanel.moc.cpp mainwindow.moc.cpp oscilloscope.moc.cpp screen.moc.cpp

QPicoscope_CXXFLAGS = $(QT_CXXFLAGS) $(AM_CXXFLAGS)
QPicoscope_CPPFLAGS = $(QT_CPPFLAGS) $(AM_CPPFLAGS) $(CFLAGS_QWT)
QPicoscope_LDFLAGS  = $(QT_LDFLAGS) $(LDFLAGS) $(QWT_LDFLAGS)
QPicoscope_LDADD    = $(QT_LIBS) $(LDADD) $(QWT_LIBADD)

BUILT_SOURCES = drawdata.moc.cpp frontpanel.moc.cpp mainwindow.moc.cpp oscilloscope.moc.cpp screen.moc.cpp

