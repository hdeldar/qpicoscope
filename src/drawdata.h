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
 * @file drawdata.h
 * @brief Declaration of draw data class.
 * @version 0.1
 * @date 2013, january 2
 * @author Vincent HERVIEUX    -   02.01.2013   -   initial creation
 */

#ifndef DRAWDATA_H
#define DRAWDATA_H

#include "oscilloscope.h"

class DrawData
{

public:
    /**
     * @brief: set data to draw
     * @param[in] channel_id: when getting multiple channels, a.k.a multiple curves, id between curves must be different
     * @param[in] x_data table of X-axis. Table has nb_points elements. Table will be copied.
     * @param[in] y_data table of Y-axis. Table has nb_points elements. Table will be copied.
     * @param[in] nb_points is the table size.
     * return : 0 if successful, -1 in case of error
     */
    virtual int8_t setData(uint8_t channel_id, double *x_data, double *y_data, uint32_t nb_points) = 0;

};

#endif
