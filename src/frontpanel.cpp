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
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStringList>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidget>
#include <QComboBox>
#include <QStatusBar>
#include <QtGui>

#include "screen.h"
#include "frontpanel.h"
#include "comborange.h"


FrontPanel::FrontPanel(QWidget *parent)
    : QWidget(parent)
{
    Acquisition::device_info_t device_info;
    QFrame *screenBox = new QFrame;
    QHBoxLayout *topLayout = new QHBoxLayout;
    QVBoxLayout *leftLayout = new QVBoxLayout;
    QVBoxLayout *screenLayout = new QVBoxLayout;
    QGridLayout *gridLayout = new QGridLayout;

    // create the oscilloscope screen
    screen = new Screen();

    // mod the front panel depending on the picoscope capabilities
    acquisition = Acquisition::get_instance();
    acquisition->setDrawData(screen);
    acquisition->get_device_info(&device_info);
    // show the detected device name in status bar for 30 seconds
    ((QMainWindow*)parent)->statusBar()->showMessage(tr(device_info.device_name), 30000);
    // Create our combo boxes with label:
    create_menu_items();

    if(device_info.nb_channels >= 1)
    {
        volt_channel_A = new ComboRange(tr("VOLT/DIV CH. A"));
        for(uint32_t i = 0; i < volt_items->size(); i++)
            volt_channel_A->setValue(i, (volt_items->at(i)).name.c_str());
        // connect volt combo to the font panel
        // front panel will then set screen values
        connect(volt_channel_A, SIGNAL(valueChanged(int)), this, SLOT(setVoltChannelAChanged(int)));
        leftLayout->addWidget(volt_channel_A);
        // set screen values
        setVoltChannelAChanged(0);
    }

    if(device_info.nb_channels >= 2)
    {
        volt_channel_B = new ComboRange(tr("VOLT/DIV CH. B"));
        for(uint32_t i = 0; i < volt_items->size(); i++)
            volt_channel_B->setValue(i, (volt_items->at(i)).name.c_str());
        // connect volt combo to the font panel
        // front panel will then set screen values
        connect(volt_channel_B, SIGNAL(valueChanged(int)), this, SLOT(setVoltChannelBChanged(int)));
        leftLayout->addWidget(volt_channel_B);
        // set screen values
        setVoltChannelBChanged(0);
    }

    time = new ComboRange(tr("TIME/DIV"));
    for(uint32_t i = 0; i < time_items->size(); i++)
        time->setValue(i, (time_items->at(i)).name.c_str());
    // connect time combo to the font panel
    // front panel will then set screen values
    connect(time, SIGNAL(valueChanged(int)), this, SLOT(setTimeChanged(int)));
    leftLayout->addWidget(time);
    // set screen values
    setTimeChanged(0);

    current = new ComboRange(tr("CURRENT"));
    for(uint32_t i = 0; i < current_items->size(); i++)
        current->setValue(i, (current_items->at(i)).name.c_str());
    // connect current combo to the font panel
    // front panel will then set screen values
    connect(current, SIGNAL(valueChanged(int)), this, SLOT(setCurrentChanged(int)));
    leftLayout->addWidget(current);
    // set screen values
    setCurrentChanged(0);

    trigger = new ComboRange(tr("TRIGGER"));
    for(uint32_t i = 0; i < trigger_items->size(); i++)
        trigger->setValue(i, (trigger_items->at(i)).name.c_str());
    // connect trigger combo to the font panel
    connect(trigger, SIGNAL(valueChanged(int)), this, SLOT(setTriggerChanged(int)));
    leftLayout->addWidget(trigger);
    // set screen values
    setTriggerChanged(0);

    screenBox->setFrameStyle(QFrame::WinPanel | QFrame::Sunken);

    (void) new QShortcut(Qt::CTRL + Qt::Key_Q, this, SLOT(close()));

    topLayout->addStretch(1);
    //topLayout->addWidget(quit);

    screenLayout->addWidget(screen);
    screenBox->setLayout(screenLayout);

    //gridLayout->addWidget(quit, 0, 0);
    gridLayout->addLayout(topLayout, 0, 1);
    gridLayout->addLayout(leftLayout, 1, 0);
    gridLayout->addWidget(screenBox, 1, 1, 2, 1);
    gridLayout->setColumnStretch(1, 10);
    setLayout(gridLayout);

}

FrontPanel::~FrontPanel()
{
    /* delete ComboRanges */
    delete volt_channel_A;
    delete volt_channel_B;
    delete current;
    delete time;
    delete trigger;

    /* delete items */
    delete volt_items;
    delete current_items;
    delete time_items;
    delete trigger_items;

    /* delete acquisition */
    delete acquisition;
}

void FrontPanel::create_menu_items()
{
    volt_item_t new_volt_item;
    time_item_t new_time_item;
    current_item_t new_current_item;
    trigger_item_t new_trigger_item;

    /* create voltage items */
    volt_items = new std::vector<volt_item_t>();
    new_volt_item.name = "5mV/div";
    new_volt_item.value = 0.005;
    volt_items->push_back(new_volt_item);
    new_volt_item.name = "10mV/div";
    new_volt_item.value = 0.01;
    volt_items->push_back(new_volt_item);
    new_volt_item.name = "20mV/div";
    new_volt_item.value = 0.02;
    volt_items->push_back(new_volt_item);
    new_volt_item.name = "50mV/div";
    new_volt_item.value = 0.05;
    volt_items->push_back(new_volt_item);
    new_volt_item.name = "100mV/div";
    new_volt_item.value = 0.1;
    volt_items->push_back(new_volt_item);
    new_volt_item.name = "200mV/div";
    new_volt_item.value = 0.2;
    volt_items->push_back(new_volt_item);
    new_volt_item.name = "500mV/div";
    new_volt_item.value = 0.5;
    volt_items->push_back(new_volt_item);
    new_volt_item.name = "1V/div";
    new_volt_item.value = 1.;
    volt_items->push_back(new_volt_item);
    new_volt_item.name = "2V/div";
    new_volt_item.value = 2.;
    volt_items->push_back(new_volt_item);

    /* create time items */
    time_items = new std::vector<time_item_t>();
    new_time_item.name = "50ns/div";
    new_time_item.value = 0.00000005;
    time_items->push_back(new_time_item);
    new_time_item.name = "100ns/div";
    new_time_item.value = 0.0000001;
    time_items->push_back(new_time_item);
    new_time_item.name = "200ns/div";
    new_time_item.value = 0.0000002;
    time_items->push_back(new_time_item);
    new_time_item.name = "500ns/div";
    new_time_item.value = 0.0000005;
    time_items->push_back(new_time_item);
    new_time_item.name = "1µs/div";
    new_time_item.value = 0.000001;
    time_items->push_back(new_time_item);
    new_time_item.name = "2µs/div";
    new_time_item.value = 0.000002;
    time_items->push_back(new_time_item);
    new_time_item.name = "5µs/div";
    new_time_item.value = 0.000005;
    time_items->push_back(new_time_item);
    new_time_item.name = "10µs/div";
    new_time_item.value = 0.00001;
    time_items->push_back(new_time_item);
    new_time_item.name = "20µs/div";
    new_time_item.value = 0.00002;
    time_items->push_back(new_time_item);
    new_time_item.name = "50µs/div";
    new_time_item.value = 0.00005;
    time_items->push_back(new_time_item);
    new_time_item.name = "100µs/div";
    new_time_item.value = 0.0001;
    time_items->push_back(new_time_item);
    new_time_item.name = "200µs/div";
    new_time_item.value = 0.0002;
    time_items->push_back(new_time_item);
    new_time_item.name = "500µs/div";
    new_time_item.value = 0.0005;
    time_items->push_back(new_time_item);
    new_time_item.name = "1ms/div";
    new_time_item.value = 0.001;
    time_items->push_back(new_time_item);
    new_time_item.name = "2ms/div";
    new_time_item.value = 0.002;
    time_items->push_back(new_time_item);
    new_time_item.name = "5ms/div";
    new_time_item.value = 0.005;
    time_items->push_back(new_time_item);
    new_time_item.name = "10ms/div";
    new_time_item.value = 0.01;
    time_items->push_back(new_time_item);
    new_time_item.name = "20ms/div";
    new_time_item.value = 0.02;
    time_items->push_back(new_time_item);
    new_time_item.name = "50ms/div";
    new_time_item.value = 0.05;
    time_items->push_back(new_time_item);
    new_time_item.name = "100ms/div";
    new_time_item.value = 0.1;
    time_items->push_back(new_time_item);
    new_time_item.name = "200ms/div";
    new_time_item.value = 0.2;
    time_items->push_back(new_time_item);
    new_time_item.name = "500ms/div";
    new_time_item.value = 0.5;
    time_items->push_back(new_time_item);
    new_time_item.name = "1s/div";
    new_time_item.value = 1.;
    time_items->push_back(new_time_item);
    new_time_item.name = "2s/div";
    new_time_item.value = 2.;
    time_items->push_back(new_time_item);
    new_time_item.name = "5s/div";
    new_time_item.value = 5.;
    time_items->push_back(new_time_item);
    new_time_item.name = "10s/div";
    new_time_item.value = 10.;
    time_items->push_back(new_time_item);
    new_time_item.name = "20s/div";
    new_time_item.value = 20.;
    time_items->push_back(new_time_item);
    new_time_item.name = "100s/div";
    new_time_item.value = 100.;
    time_items->push_back(new_time_item);
    new_time_item.name = "200s/div";
    new_time_item.value = 200.;
    time_items->push_back(new_time_item);
    new_time_item.name = "500s/div";
    new_time_item.value = 500.;
    time_items->push_back(new_time_item);
    new_time_item.name = "1000s/div";
    new_time_item.value = 1000.;
    time_items->push_back(new_time_item);

    /* create current items */
    current_items = new std::vector<current_item_t>();
    new_current_item.name = "AC";
    new_current_item.value = E_CURRENT_AC;
    current_items->push_back(new_current_item);
    new_current_item.name = "DC";
    new_current_item.value = E_CURRENT_DC;
    current_items->push_back(new_current_item);

    /* create trigger items */
    trigger_items = new std::vector<trigger_item_t>();
    new_trigger_item.name = "Auto";
    new_trigger_item.value = E_TRIGGER_AUTO;
    trigger_items->push_back(new_trigger_item);
    //TODO new_trigger_item.name = "Rising";
    //TODO new_trigger_item.value = E_TRIGGER_RISING;
    //TODO trigger_items->push_back(new_trigger_item);
    //TODO new_trigger_item.name = "Falling";
    //TODO new_trigger_item.value = E_TRIGGER_FALLING;
    //trigger_items->push_back(new_trigger_item);

}

void FrontPanel::setVoltChannelAChanged(int comboIndex)
{
    screen->setVoltCaliber((volt_items->at(comboIndex)).value);
    acquisition->stop();
    acquisition->set_voltages(Acquisition::CHANNEL_A, (volt_items->at(comboIndex)).value);
    acquisition->start();
}

void FrontPanel::setVoltChannelBChanged(int comboIndex)
{
    // A is the main channel to rescale graphics. So B is not rescaling:
    //screen->setVoltCaliber((volt_items->at(comboIndex)).value);
    acquisition->stop();
    acquisition->set_voltages(Acquisition::CHANNEL_B, (volt_items->at(comboIndex)).value);
    acquisition->start();
}

void FrontPanel::setTimeChanged(int comboIndex)
{
    screen->setTimeCaliber((time_items->at(comboIndex)).value);
    acquisition->stop();
    acquisition->set_timebase((time_items->at(comboIndex)).value);
    acquisition->start();
}

void FrontPanel::setCurrentChanged(int comboIndex)
{
    screen->setCurrent((current_items->at(comboIndex)).value);
    // TODO set acquisition accordingly
}

void FrontPanel::setTriggerChanged(int comboIndex)
{
    screen->setTrigger((trigger_items->at(comboIndex)).value);
    // TODO set acquisition accordingly
}
