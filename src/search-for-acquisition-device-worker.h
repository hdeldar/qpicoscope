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
 * @file search-for-acquisition-device-worker.h
 * @brief Declaration of SearchForAcquisitionDeviceWorker class.
 * SearchForAcquisitionDeviceWorker will be put in a QThread 
 * waiting for an acquisition device to be connected
 * @version 0.1
 * @date 2013, april 22
 * @author Vincent HERVIEUX    -   04.22.2013   -   initial creation
 */

#ifndef SEARCH_FOR_ACQUISITION_DEVICE_WORKER_H
#define SEARCH_FOR_ACQUISITION_DEVICE_WORKER_H

#include <QObject>
#include "oscilloscope.h"

class FrontPanel;

/** Thread searching for acquisition device at startup till it is connected */
class SearchForAcquisitionDeviceWorker : public QObject
{
 Q_OBJECT

 public:
    SearchForAcquisitionDeviceWorker(FrontPanel *parent);

 public slots:
    /** @brief thread searching for acquisition device */
    void searchForAcquisitionDevice(void);
    void stopSearchForAcquisitionDevice(void);

 private:
    FrontPanel* parent_m;
 signals:
    void newStatusBarMessage(QString text);
};

#endif
