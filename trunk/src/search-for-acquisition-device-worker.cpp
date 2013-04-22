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
 * @file search-for-acquisition-device-worker.cpp
 * @brief Definition of SearchForAcquisitionDeviceWorker class.
 * SearchForAcquisitionDeviceWorker will be put in a QThread 
 * waiting for an acquisition device to be connected
 * @version 0.1
 * @date 2013, april 22
 * @author Vincent HERVIEUX    -   04.22.2013   -   initial creation
 */
#include "search-for-acquisition-device-worker.h"

#include <QMainWindow>
#include <QStatusBar>
#include "acquisition.h"
#include "comborange.h"
#include "frontpanel.h"
#include "screen.h"

SearchForAcquisitionDeviceWorker::SearchForAcquisitionDeviceWorker(FrontPanel* parent):
 parent_m(parent)
{
}

void SearchForAcquisitionDeviceWorker::searchForAcquisitionDevice(void)
{
    //FrontPanel* parent = (FrontPanel*)_frontpanel;
    Acquisition* device = NULL;
    Acquisition::device_info_t device_info;
    bool running = true;
    // mod the front panel depending on the picoscope capabilities
    memset(&device_info, 0, sizeof(Acquisition::device_info_t));
    do
    {
        device = Acquisition::get_instance();
        if(NULL == device)
        {
            ERROR("Acquisition::get_instance returned NULL.\n");
            snprintf(device_info.device_name, DEVICE_NAME_MAX ,"No detected device...!");
            //((QMainWindow*)(parent_m->parent_m))->statusBar()->showMessage(tr(device_info.device_name), 1000);
            emit newStatusBarMessage(tr(device_info.device_name));
            sleep(1);
        }
    }while((NULL == device) && running);

    pthread_mutex_lock(&parent_m->acquisitionLock_m);
    parent_m->acquisition_m = device;
    parent_m->acquisition_m->setDrawData(parent_m->screen_m);
    parent_m->acquisition_m->get_device_info(&device_info);
    // show the detected device name in status bar
    emit newStatusBarMessage(tr(device_info.device_name));
    //((QMainWindow*)(parent_m->parent_m))->statusBar()->showMessage(tr(device_info.device_name), 30000);
    if(device_info.nb_channels >= 1)
    {

        parent_m->acquisition_m->set_voltages(Acquisition::CHANNEL_A, (parent_m->volt_items_m->back()).value);
        parent_m->volt_channel_A_m->setCurrentIndex(parent_m->volt_items_m->size() - 1);
        // set screen values
        parent_m->screen_m->setVoltCaliber((parent_m->volt_items_m->back()).value);
        
    }
    else
    {
        // stupid if a device has no channel...
        ERROR("This device has no channel\n");
        parent_m->volt_channel_A_m->setVisible(false);
    }

    if(device_info.nb_channels >= 2)
    {
        parent_m->acquisition_m->set_voltages(Acquisition::CHANNEL_B, (parent_m->volt_items_m->back()).value);
        parent_m->volt_channel_B_m->setCurrentIndex(parent_m->volt_items_m->size() - 1);
    }
    else
    {
        parent_m->volt_channel_B_m->setVisible(false);
    }
    parent_m->acquisition_m->set_timebase((parent_m->time_items_m->at(0)).value);
    parent_m->acquisition_m->start();
    pthread_mutex_unlock(&parent_m->acquisitionLock_m);
}

void SearchForAcquisitionDeviceWorker::stopSearchForAcquisitionDevice(void)
{
}
