# The final program is called "myapp"
bin_PROGRAMS = QPicoscope

# You have two .cpp files you wrote, myapp.cpp and another.cpp
# Remember to include the name of the resource file with the .cpp extension.
QPicoscope_SOURCES = \
	myapp.cpp \
	another.cpp \
    resources.cpp

# You have one .h file, it's called myapp.h. Therefore, here I list
# its mocced name, moc_myapp.cpp.
nodist_QPicoscope_SOURCES = \
	moc_myapp.cpp

# This is to ensure the myapp.h file is distributed in your myapp-0.1.tar.gz
# I also have a resources.qrc I need to include. The png is used in myapp.
EXTRA_DIST = \
	$(nodist_QPicoscope_SOURCES:moc_%.cpp=%.h) \
	resources.qrc myapp.png

# This rule lets GNU make create any moc_*.cpp from the equivalent *.h
moc_%.cpp: %.h
	moc $< -o $@

# Adjust this line according to your resources.qrc
resources.cpp : resources.qrc octave-gui.png
	rcc resources.qrc -o resources.cpp

# This line ensures that generated moc_*.cpp and resources.cpp files are
# deleted when we make clean.
CLEANFILES = $(filter moc_%.cpp,$(QPicoscope_SOURCES)) resources.cpp
