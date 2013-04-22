include $(top_srcdir)/build-aux/autotroll.mk

# For a program:
bin_PROGRAMS = QPicoscope
QPicoscope_SOURCES = 	acquisition2000.cpp  \
			acquisition3000.cpp  \
			acquisition.cpp  \
			comborange.cpp  \
			frontpanel.cpp  \
			main.cpp  \
			mainwindow.cpp  \
			screen.cpp \
			search-for-acquisition-device-worker.cpp \
			comborange.h  \
			comborange.moc.cpp \
			acquisition.h  \
			acquisition.moc.cpp \
			drawdata.h \
			drawdata.moc.cpp \
			frontpanel.h \
			frontpanel.moc.cpp \
			mainwindow.h \
			mainwindow.moc.cpp \
			oscilloscope.h \
			oscilloscope.moc.cpp \
			screen.h \
			screen.moc.cpp \
			search-for-acquisition-device-worker.h \
			search-for-acquisition-device-worker.moc.cpp

QPicoscope_CXXFLAGS = $(QT_CXXFLAGS) $(AM_CXXFLAGS) -g -Wall
QPicoscope_CPPFLAGS = $(QT_CPPFLAGS) $(AM_CPPFLAGS) $(CFLAGS_QWT)
QPicoscope_LDFLAGS  = $(QT_LDFLAGS) $(LDFLAGS) $(QWT_LDFLAGS)
QPicoscope_LDADD    = $(QT_LIBS) $(LDADD) $(QWT_LIBADD)

BUILT_SOURCES = drawdata.moc.cpp \
		frontpanel.moc.cpp \
		mainwindow.moc.cpp \
		oscilloscope.moc.cpp \
		screen.moc.cpp

