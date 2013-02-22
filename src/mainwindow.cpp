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
 * @file mainwindow.cpp
 * @brief Definition of MainWindow class.
 * @version 0.1
 * @date 2012, november 28
 * @author Vincent HERVIEUX    -   11.28.2012   -   initial creation
 */
#include "mainwindow.h"

MainWindow::MainWindow()
{

    createActions();
    createMenus();
    frontpanel_m = new FrontPanel(this);    
    setCentralWidget(frontpanel_m);
}

MainWindow::~MainWindow()
{
    delete frontpanel_m;
}


void MainWindow::about()
{
    QMessageBox msgBox(this);
    QDesktopWidget win;
    msgBox.setWindowTitle("About QPicoscope");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(tr(ABOUT_QPICOSCOPE));
    msgBox.show();
    msgBox.move( win.width() / 2 - msgBox.width() / 2, win.height() / 2 - msgBox.height() / 2 );
    msgBox.exec();
}

void MainWindow::aboutQt()
{
}

void MainWindow::credits()
{
    QMessageBox msgBox(this);
    QDesktopWidget win;
    msgBox.setWindowTitle("Credits");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(tr(CREDITS_QPICOSCOPE));
    msgBox.show();
    msgBox.move( win.width() / 2 - msgBox.width() / 2, win.height() / 2 - msgBox.height() / 2 );
    msgBox.exec();
}

void MainWindow::createMenus()
 {
     fileMenu_m = menuBar()->addMenu(tr("&File"));
     fileMenu_m->addAction(exitAct_m);

     helpMenu_m = menuBar()->addMenu(tr("&Help"));
     helpMenu_m->addAction(aboutAct_m);
     helpMenu_m->addAction(aboutQtAct_m);
     helpMenu_m->addAction(creditsAct_m);

 }

void MainWindow::createActions()
{
     exitAct_m = new QAction(tr("E&xit"), this);
     exitAct_m->setShortcuts(QKeySequence::Quit);
     exitAct_m->setStatusTip(tr("Exit the application"));
     connect(exitAct_m, SIGNAL(triggered()), this, SLOT(close()));

     aboutAct_m = new QAction(tr("&About"), this);
     aboutAct_m->setStatusTip(tr("Show the application's About box"));
     connect(aboutAct_m, SIGNAL(triggered()), this, SLOT(about()));

     aboutQtAct_m = new QAction(tr("About &Qt"), this);
     aboutQtAct_m->setStatusTip(tr("Show the Qt library's About box"));
     connect(aboutQtAct_m, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
     connect(aboutQtAct_m, SIGNAL(triggered()), this, SLOT(aboutQt()));
     
     creditsAct_m = new QAction(tr("&Credits"), this);
     creditsAct_m->setStatusTip(tr("Show the application's credits box"));
     connect(creditsAct_m, SIGNAL(triggered()), this, SLOT(credits()));
}
