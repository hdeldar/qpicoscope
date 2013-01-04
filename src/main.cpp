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
 * @file main.cpp
 * @brief Main program entry point.
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */

#include <QApplication>

#include "mainwindow.h"

/** @brief all programs have a start point... */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow mainwindow;
    
    
    mainwindow.setGeometry(100, 100, 800, 600);
    mainwindow.show();
    return app.exec();
}
