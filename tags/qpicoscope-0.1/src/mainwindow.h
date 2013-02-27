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
 * @file mainwindow.h
 * @brief Declaration of MainWindow class.
 * MainWindow aims to be the application main window
 * @version 0.1
 * @date 2012, november 28
 * @author Vincent HERVIEUX    -   11.28.2012   -   initial creation
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>

#include "frontpanel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    /**
     * @brief constructor
     */
    MainWindow();
    /**
     * @brief destructor
     */
    virtual ~MainWindow();

private slots:
    void about();
    void aboutQt();
    void credits();

private:
    void createActions();
    void createMenus();

    FrontPanel* frontpanel_m;
    QMenu *fileMenu_m;
    QMenu *helpMenu_m;
    QAction *exitAct_m;
    QAction *aboutAct_m;
    QAction *aboutQtAct_m;
    QAction *creditsAct_m;
};

#endif
