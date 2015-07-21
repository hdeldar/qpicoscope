# I - INTRODUCTION #

QPicoscope is a QT frontend for Picoscope devices from Picotech.

It actually should support devices from series 2000 and 3000.

This program is distributed under the GNU LESSER GENERAL PUBLIC LICENSE. See COPYING and COPYING.LESSER to get more info on this license.

# II - CREDITS #

QPicoscope is using:
- Qt. Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies). Qt is a Nokia product. See qt.nokia.com for more information.
- Qwt. Copyright (C) 1997 Josef Wilgen; Copyright (C) 2002 Uwe Rathmann.
- AutoTroll. Copyright (C) 2006 Benoit Sigoure


# III - INSTALLATION #

First you will need to install one of the Picotech libraries.

The libraries can be downloaded from the official Picotech website: http://www.picotech.com/linux.html

Or are available within the lib directory.

To support both 2000 and 3000 series, install libps2\_3000-x.x.x.x-x.i386 or libps2\_3000-x.x.x.x-x.x86\_64 depending on your system architecture.

To support only 2000 serie, install libps2000-x.x.x.x-x.i386 or libps2000-x.x.x.x-x.x86\_64  depending on your system architecture.

In library folder you will find :
- a binary package to install .deb or .rpm depending on your distribution.
- a udev rule to add on your machine (probably in /etc/udev/rules.d/95-pico.rules), please have a look at this udev rule, while also indicates how to add a pico group to your machine and how to add your current user to this pico group.

Once Picotech library is install you can compile QPicoscope using:

- Autotools (see §III.1)
- qmake (see §III.2)

## III.1 - INSTALLATION USING AUTOTOOLS ##

run the following commands:
./autogen.sh
./configure
make
make install

## III.2 - INSTALLATION QMAKE ##

run the following commands:
cd src
qmake-qt4 -set QMAKEFEATURES /usr/share/qt4/mkspecs/features
qmake-qt4 qpicoscope.pro
make


# IV - BUG REPORT #

QPicoscope is free and might be full of bugs!

To report bugs, please visit http://code.google.com/p/qpicoscope/issues/list


# V - KNOWN ISSUES #

Please visit http://code.google.com/p/qpicoscope/issues/list

Do not hesitate to raise new issue!