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
 * @file frontpanel.cpp
 * @brief Definition of FrontPanel class.
 * FrontPanel aims to be the HMI of the oscilloscope
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */
#include <QApplication>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStringList>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QWidget>
#include <QComboBox>
#include <QStatusBar>
#include <QtGui>

#include "screen.h"
#include "frontpanel.h"
#include "comborange.h"


FrontPanel::FrontPanel(QWidget *parent)
    : QWidget(parent),
      parent_m(parent)
{
    Acquisition::device_info_t device_info;
    QFrame *screenBox = new QFrame;
    QHBoxLayout *topLayout = new QHBoxLayout;
    QVBoxLayout *leftLayout = new QVBoxLayout;
    QVBoxLayout *screenLayout = new QVBoxLayout;
    QGridLayout *gridLayout = new QGridLayout;

    /* initialize acquisition */
    acquisition_m = NULL;
    pthread_mutex_init(&acquisitionLock_m, NULL);
    
    /* initialize ComboRanges */
    volt_channel_A_m = NULL;
    volt_channel_B_m = NULL;
    current_m = NULL;
    time_m = NULL;
    trigger_m = NULL;

    /* initialize items */
    volt_items_m = NULL;
    current_items_m = NULL;
    time_items_m = NULL;
    trigger_items_m = NULL;

    /* initialize spinbox */
    trigger_value_m = NULL;

    /* create the oscilloscope screen */
    screen_m = new Screen();

    // mod the front panel depending on the picoscope capabilities
    memset(&device_info, 0, sizeof(Acquisition::device_info_t));
    device_info.nb_channels = 2;
    // Create our combo boxes with label:
    create_menu_items();

    if(device_info.nb_channels >= 1)
    {
        volt_channel_A_m = new ComboRange(tr("VOLT/DIV CH. A"));
        for(uint32_t i = 0; i < volt_items_m->size(); i++)
            volt_channel_A_m->setValue(i, (volt_items_m->at(i)).name.c_str());
        volt_channel_A_m->setCurrentIndex(volt_items_m->size() - 1);
        // set screen values
        if(NULL != screen_m)
        {
            screen_m->setVoltCaliber((volt_items_m->back()).value);
        }
        // connect volt combo to the font panel
        // front panel will then set screen values
        connect(volt_channel_A_m, SIGNAL(valueChanged(int)), this, SLOT(setVoltChannelAChanged(int)));
        leftLayout->addWidget(volt_channel_A_m);
        
    }

    if(device_info.nb_channels >= 2)
    {
        volt_channel_B_m = new ComboRange(tr("VOLT/DIV CH. B"));
        for(uint32_t i = 0; i < volt_items_m->size(); i++)
            volt_channel_B_m->setValue(i, (volt_items_m->at(i)).name.c_str());
        volt_channel_B_m->setCurrentIndex(volt_items_m->size() - 1);
        // connect volt combo to the font panel
        // front panel will then set screen values
        connect(volt_channel_B_m, SIGNAL(valueChanged(int)), this, SLOT(setVoltChannelBChanged(int)));
        leftLayout->addWidget(volt_channel_B_m);
    }


    time_m = new ComboRange(tr("TIME/DIV"));
    for(uint32_t i = 0; i < time_items_m->size(); i++)
        time_m->setValue(i, (time_items_m->at(i)).name.c_str());
    // set screen values
    if(NULL != screen_m)
    {
        screen_m->setTimeCaliber((time_items_m->at(0)).value);
    }
    time_m->setCurrentIndex(0);
    // connect time combo to the font panel
    // front panel will then set screen values
    connect(time_m, SIGNAL(valueChanged(int)), this, SLOT(setTimeChanged(int)));
    leftLayout->addWidget(time_m);

    current_m = new ComboRange(tr("CURRENT"));
    for(uint32_t i = 0; i < current_items_m->size(); i++)
        current_m->setValue(i, (current_items_m->at(i)).name.c_str());
    // connect current combo to the font panel
    // front panel will then set screen values
    connect(current_m, SIGNAL(valueChanged(int)), this, SLOT(setCurrentChanged(int)));
    leftLayout->addWidget(current_m);
    // set screen values
    setCurrentChanged(0);

    trigger_m = new ComboRange(tr("TRIGGER"));
    for(uint32_t i = 0; i < trigger_items_m->size(); i++)
        trigger_m->setValue(i, (trigger_items_m->at(i)).name.c_str());
    // connect trigger combo to the font panel
    connect(trigger_m, SIGNAL(valueChanged(int)), this, SLOT(setTriggerChanged(int)));
    leftLayout->addWidget(trigger_m);

    trigger_value_m = new QDoubleSpinBox;
    trigger_value_m->setRange(-5.0, 5.0);
    trigger_value_m->setSingleStep(0.01);
    trigger_value_m->setValue(0.0);
    trigger_value_m->hide();
    connect(trigger_value_m, SIGNAL(valueChanged(double)), this, SLOT(setTriggerChanged(double)));
    leftLayout->addWidget(trigger_value_m);

    // set screen values
    setTriggerChanged(0);

    screenBox->setFrameStyle(QFrame::WinPanel | QFrame::Sunken);

    (void) new QShortcut(Qt::CTRL + Qt::Key_Q, this, SLOT(close()));

    topLayout->addStretch(1);

    screenLayout->addWidget(screen_m);
    screenBox->setLayout(screenLayout);

    gridLayout->addLayout(topLayout, 0, 1);
    gridLayout->addLayout(leftLayout, 1, 0);
    gridLayout->addWidget(screenBox, 1, 1, 2, 1);
    gridLayout->setColumnStretch(1, 10);
    setLayout(gridLayout);


    /* initialize acquisition search */
    QThread* searchForAcquisitionDeviceThread = new QThread(this);;
    SearchForAcquisitionDeviceWorker* searchForAcquisitionDeviceWorker = new SearchForAcquisitionDeviceWorker(this);    
    connect(searchForAcquisitionDeviceThread, 
            SIGNAL(started()), 
            searchForAcquisitionDeviceWorker, 
            SLOT(searchForAcquisitionDevice()));
    connect(searchForAcquisitionDeviceThread, 
            SIGNAL(finished()), 
            searchForAcquisitionDeviceWorker, 
            SLOT(stopSearchForAcquisitionDevice()));
    connect(searchForAcquisitionDeviceWorker,
            SIGNAL(newStatusBarMessage(QString)), 
            this,
            SLOT(setStatusBarMessage(QString)));
    searchForAcquisitionDeviceWorker->moveToThread(searchForAcquisitionDeviceThread);
    searchForAcquisitionDeviceThread->start();
}

FrontPanel::~FrontPanel()
{
    /* delete ComboRanges */
    if( NULL != volt_channel_A_m )
        delete volt_channel_A_m;
    if( NULL != volt_channel_B_m )
        delete volt_channel_B_m;
    if( NULL != current_m )
        delete current_m;
    if( NULL != time_m )
        delete time_m;
    if( NULL != trigger_m )
        delete trigger_m;

    /* delete items */
    if( NULL != volt_items_m )
        delete volt_items_m;
    if( NULL != current_items_m )
        delete current_items_m;
    if( NULL != time_items_m )
        delete time_items_m;
    if( NULL != trigger_items_m )
        delete trigger_items_m;
    if( NULL != trigger_value_m )
        delete trigger_value_m;

    /* delete acquisition */
    if(NULL != acquisition_m)
    {
        acquisition_m->stop();
        delete acquisition_m;
        acquisition_m = NULL;
    }
}

void FrontPanel::create_menu_items()
{
    volt_item_t new_volt_item;
    time_item_t new_time_item;
    current_item_t new_current_item;
    trigger_item_t new_trigger_item;

    /* create voltage items */
    volt_items_m = new std::vector<volt_item_t>();
    new_volt_item.name = "5mV/div";
    new_volt_item.value = 0.005;
    volt_items_m->push_back(new_volt_item);
    new_volt_item.name = "10mV/div";
    new_volt_item.value = 0.01;
    volt_items_m->push_back(new_volt_item);
    new_volt_item.name = "20mV/div";
    new_volt_item.value = 0.02;
    volt_items_m->push_back(new_volt_item);
    new_volt_item.name = "50mV/div";
    new_volt_item.value = 0.05;
    volt_items_m->push_back(new_volt_item);
    new_volt_item.name = "100mV/div";
    new_volt_item.value = 0.1;
    volt_items_m->push_back(new_volt_item);
    new_volt_item.name = "200mV/div";
    new_volt_item.value = 0.2;
    volt_items_m->push_back(new_volt_item);
    new_volt_item.name = "500mV/div";
    new_volt_item.value = 0.5;
    volt_items_m->push_back(new_volt_item);
    new_volt_item.name = "1V/div";
    new_volt_item.value = 1.;
    volt_items_m->push_back(new_volt_item);
    new_volt_item.name = "2V/div";
    new_volt_item.value = 2.;
    volt_items_m->push_back(new_volt_item);

    /* create time items */
    time_items_m = new std::vector<time_item_t>();
#if 0
    new_time_item.name = "50ns/div";
    new_time_item.value = 0.00000005;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "100ns/div";
    new_time_item.value = 0.0000001;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "200ns/div";
    new_time_item.value = 0.0000002;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "500ns/div";
    new_time_item.value = 0.0000005;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "1µs/div";
    new_time_item.value = 0.000001;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "2µs/div";
    new_time_item.value = 0.000002;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "5µs/div";
    new_time_item.value = 0.000005;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "10µs/div";
    new_time_item.value = 0.00001;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "20µs/div";
    new_time_item.value = 0.00002;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "50µs/div";
    new_time_item.value = 0.00005;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "100µs/div";
    new_time_item.value = 0.0001;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "200µs/div";
    new_time_item.value = 0.0002;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "500µs/div";
    new_time_item.value = 0.0005;
    time_items_m->push_back(new_time_item);
#endif
    new_time_item.name = "1ms/div";
    new_time_item.value = 0.001;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "2ms/div";
    new_time_item.value = 0.002;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "5ms/div";
    new_time_item.value = 0.005;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "10ms/div";
    new_time_item.value = 0.01;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "20ms/div";
    new_time_item.value = 0.02;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "50ms/div";
    new_time_item.value = 0.05;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "100ms/div";
    new_time_item.value = 0.1;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "200ms/div";
    new_time_item.value = 0.2;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "500ms/div";
    new_time_item.value = 0.5;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "1s/div";
    new_time_item.value = 1.;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "2s/div";
    new_time_item.value = 2.;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "5s/div";
    new_time_item.value = 5.;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "10s/div";
    new_time_item.value = 10.;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "20s/div";
    new_time_item.value = 20.;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "100s/div";
    new_time_item.value = 100.;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "200s/div";
    new_time_item.value = 200.;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "500s/div";
    new_time_item.value = 500.;
    time_items_m->push_back(new_time_item);
    new_time_item.name = "1000s/div";
    new_time_item.value = 1000.;
    time_items_m->push_back(new_time_item);

    /* create current items */
    current_items_m = new std::vector<current_item_t>();
    new_current_item.name = "AC";
    new_current_item.value = E_CURRENT_AC;
    current_items_m->push_back(new_current_item);
    new_current_item.name = "DC";
    new_current_item.value = E_CURRENT_DC;
    current_items_m->push_back(new_current_item);

    /* create trigger items */
    trigger_items_m = new std::vector<trigger_item_t>();
    new_trigger_item.name = "Auto";
    new_trigger_item.value = E_TRIGGER_AUTO;
    trigger_items_m->push_back(new_trigger_item);
    new_trigger_item.name = "Rising";
    new_trigger_item.value = E_TRIGGER_RISING;
    trigger_items_m->push_back(new_trigger_item);
    new_trigger_item.name = "Falling";
    new_trigger_item.value = E_TRIGGER_FALLING;
    trigger_items_m->push_back(new_trigger_item);

}

void FrontPanel::setVoltChannelAChanged(int comboIndex)
{
    DEBUG("Combo index %d\n", comboIndex);
    screen_m->setVoltCaliber((volt_items_m->at(comboIndex)).value);
    if( NULL != acquisition_m )
    {
        acquisition_m->stop();
        acquisition_m->set_voltages(Acquisition::CHANNEL_A, (volt_items_m->at(comboIndex)).value);
        acquisition_m->start();
    }
}

void FrontPanel::setVoltChannelBChanged(int comboIndex)
{
    DEBUG("Combo index %d\n", comboIndex);
    // A is the main channel to rescale graphics. So B is not rescaling:
    //screen_m->setVoltCaliber((volt_items_m->at(comboIndex)).value);
    if( NULL != acquisition_m )
    {
        acquisition_m->stop();
        acquisition_m->set_voltages(Acquisition::CHANNEL_B, (volt_items_m->at(comboIndex)).value);
        acquisition_m->start();
    }
}

void FrontPanel::setTimeChanged(int comboIndex)
{
    DEBUG("Combo index %d\n", comboIndex);
    screen_m->setTimeCaliber((time_items_m->at(comboIndex)).value);
    if( NULL != acquisition_m )
    {
        acquisition_m->stop();
        acquisition_m->set_timebase((time_items_m->at(comboIndex)).value);
        acquisition_m->start();
    }
}

void FrontPanel::setCurrentChanged(int comboIndex)
{
    DEBUG("Combo index %d\n", comboIndex);
    screen_m->setCurrent((current_items_m->at(comboIndex)).value);
    // TODO set acquisition accordingly
    if ( NULL != acquisition_m )
    {
        acquisition_m->stop();
        acquisition_m->set_DC_coupled((current_items_m->at(comboIndex)).value);
        acquisition_m->start();
    }
}

void FrontPanel::setTriggerChanged(int comboIndex)
{
    DEBUG("Combo index %d\n", comboIndex);
    screen_m->setTrigger((trigger_items_m->at(comboIndex)).value);
    if( NULL == trigger_value_m )
    {
        ERROR("trigger_value is NULL.\n");
        return;
    }
    if( NULL != acquisition_m )
    {
        acquisition_m->stop();
        acquisition_m->set_trigger((trigger_items_m->at(comboIndex)).value, trigger_value_m->value());
        /* If Auto trigger is set, hide trigger input 
         * and if not set, show trigger input dialog.
         */
        if(((trigger_items_m->at(comboIndex)).value == E_TRIGGER_AUTO) && (trigger_value_m->isHidden() == false))
        {
            trigger_value_m->hide();
        }
        else if(((trigger_items_m->at(comboIndex)).value != E_TRIGGER_AUTO) && (trigger_value_m->isHidden() == true))
        {
            trigger_value_m->show();
        }
        acquisition_m->start();
    }
}

void FrontPanel::setTriggerChanged(double trigger_value)
{
    (void)trigger_value;
    setTriggerChanged(trigger_m->value());
}

void FrontPanel::setStatusBarMessage(QString text)
{
  ((QMainWindow*)(parent_m))->statusBar()->showMessage(text, 30000);
}
