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
 * @file frontpanel.h
 * @brief Declaration of FrontPanel class.
 * FrontPanel aims to be the HMI of the oscilloscope
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */

#ifndef FRONTPANEL_H
#define FRONTPANEL_H

#include <QWidget>
#include <string>
#include <vector>

#include "oscilloscope.h"
#include "acquisition.h"

class ComboRange;
class Screen;

class FrontPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief constructor
     * @param[in] parent widget pointer
     */
    FrontPanel(QWidget *parent = 0);

    ~FrontPanel();

protected slots:
    void setVoltChannelAChanged(int);
    void setVoltChannelBChanged(int);
    void setTimeChanged(int);
    void setCurrentChanged(int);
    void setTriggerChanged(int);

private:
    /** @brief create menu items */
    void create_menu_items();
    /** @brief screen of the front panel */
    Screen *screen;
    /** @brief Acquisition engine of the oscilloscope */
    Acquisition* acquisition;
    /** @brief voltage selection on the front panel */
    ComboRange *volt_channel_A;
    ComboRange *volt_channel_B;
    typedef struct
    {
        std::string name;
        double value;
    }volt_item_t;
    std::vector<volt_item_t> *volt_items;
    /** @brief timing selection on the front panel */
    ComboRange *time;
    typedef struct
    {
        std::string name;
        double value;
    }time_item_t;
    std::vector<time_item_t> *time_items;
    /** @brief current type selection on the front panel */
    ComboRange *current;
    typedef struct
    {
        std::string name;
        current_e value;
    }current_item_t;
    std::vector<current_item_t> *current_items;
    /** @brief trigger type selection on the front panel */
    ComboRange *trigger;
    typedef struct
    {
        std::string name;
        trigger_e value;
    }trigger_item_t;
    std::vector<trigger_item_t> *trigger_items;

};

#endif
