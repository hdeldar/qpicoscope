/*****************************************************************************
*   Copyright 2012 Vincent HERVIEUX
*
*   This file is part of QPicoscope.
*
*   QPicoscope is free software: you can redistribute it and/or modify
*   it under the terms of the GNU Lesser General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   QPicoscope is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public License
*   along with QPicoscope in files COPYING.LESSER and COPYING.
*   If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
/**
 * @file oscilloscope.h
 * @brief Declaration of program wide types.
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */

#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <stdint.h>
#include <stdio.h>
#include <limits.h>

/*!!! TODO remove this flag while testing with HW!!!*/
//#define TEST_WITHOUT_HW

#define ABOUT_QPICOSCOPE     "<p align='center'>Copyright 2012 Vincent HERVIEUX<br>"\
                             "<a href='mailto:vincent.hervieux@free.fr?Subject=QPicoscope'>vincent.hervieux@free.fr</a></p>"\
                             "<p align='justify'><br>"\
                             "Qpicoscope is a Qt frontend for Picoscope 2000-3000 series<br>"\
                             "from <a href='http://www.picotech.com'>Pico Tech</a>.<br>"\
                             "<br>"\
                             "QPicoscope is free software: you can redistribute it and/or modify<br>"\
                             "it under the terms of the GNU Lesser General Public License as published by<br>"\
                             "the Free Software Foundation, either version 3 of the License, or<br>"\
                             "any later version.<br>"\
                             "<br>"\
                             "QPicoscope is distributed in the hope that it will be useful,<br>"\
                             "but WITHOUT ANY WARRANTY; without even the implied warranty of<br>"\
                             "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the<br>"\
                             "GNU Lesser General Public License for more details.<br>"\
                             "<br>"\
                             "You should have received a copy of the GNU Lesser General Public License<br>"\
                             "along with QPicoscope in files COPYING.LESSER and COPYING.<br>"\
                             "If not, see <a href='http://www.gnu.org/licenses'>http://www.gnu.org/licenses</a>.<br></p>"


#define CREDITS_QPICOSCOPE   "<p align='justify'>QPicoscope si using:<br>"\
                             "- Qt. Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).<br>"\
                             "  Qt is a Nokia product. See qt.nokia.com for more information.<br>"\
                             "- Qwt. Copyright (C) 1997 Josef Wilgen; Copyright (C) 2002 Uwe Rathmann.<br>"\
                             "- AutoTroll. Copyright (C) 2006  Benoit Sigoure <benoit.sigoure@lrde.epita.fr.</p>"
typedef enum
{
    E_CURRENT_AC = 0,
    E_CURRENT_DC
}current_e;

typedef enum
{
    E_TRIGGER_AUTO = 0,
    E_TRIGGER_RISING,
    E_TRIGGER_FALLING
}trigger_e;

#define DEBUG(...)     do{ fprintf(stderr, "%s\t- %s:\t[%d]\tDEBUG: ",__FILE__, __FUNCTION__,__LINE__); fprintf(stderr, __VA_ARGS__); }while(0)
#define ERROR(...)     do{ fprintf(stderr, "%s\t- %s:\t[%d]\tERROR: ",__FILE__, __FUNCTION__,__LINE__); fprintf(stderr, __VA_ARGS__); }while(0)
#define WARNING(...)   do{ fprintf(stderr, "%s\t- %s:\t[%d]\tWARNING: ",__FILE__, __FUNCTION__,__LINE__); fprintf(stderr, __VA_ARGS__); }while(0)

#endif // OSCILLOSCOPE_H
