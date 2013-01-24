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
 * @file screen.h
 * @brief Declaration of Screen class.
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */

#ifndef SCREEN_H
#define SCREEN_H

//#include <QWidget>
#include <qwt_plot.h>
#include <qwt_plot_curve.h> 

#include "oscilloscope.h"
#include "drawdata.h"

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class Screen : public QwtPlot, public DrawData
{
    Q_OBJECT

public:
    /**
     * @brief constructor
     * @param[in] parent widget pointer
     */
    Screen(QWidget *parent = 0);
    /**
     * @brief get voltage caliber
     * @return current voltage caliber over a double
     */
    double voltCaliber() const { return currentVoltCaliber; }
    /**
     * @brief get time caliber
     * @return current time caliber over a double
     */
    double timeCaliber() const { return currentTimeCaliber; }
    /**
     * @brief get current type set
     * @return current current type set
     */
    current_e current() const { return currentCurrent; }
    /**
     * @brief get trigger type set
     * @return current trigger type set
     */
    trigger_e trigger() const { return currentTrigger; }

    //QSize sizeHint() const;
 
    /**
     * @brief: set data to draw
     * @param[in] channel_id: when getting multiple channels, a.k.a multiple curves, id between curves must be different
     * @param[in] x_data table of X-axis. Table has nb_points elements. Table will be copied.
     * @param[in] y_data table of Y-axis. Table has nb_points elements. Table will be copied.
     * @param[in] nb_points is the table size.
     * return : 0 if successful, -1 in case of error
     */
    int8_t setData(uint8_t channel_id, double *x_data, double *y_data, uint32_t nb_points);

public slots:
    /**
     * @brief set voltage caliber
     * @param[in] voltage caliber over a double to set
     */
    void setVoltCaliber(double voltCaliber);
    /**
     * @brief set time caliber
     * @param[in] time caliber over a double to set
     */
    void setTimeCaliber(double timeCaliber);
    /**
     * @brief set current type
     * @param[in] current type to set
     */
    void setCurrent(current_e current);
    /**
     * @brief set trigger type
     * @param[in] trigger type to set
     */
    void setTrigger(trigger_e trigger);

private slots:

signals:

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:

    double currentVoltCaliber;
    double currentTimeCaliber;
    current_e currentCurrent;
    trigger_e currentTrigger;
    void initGradient();
    /* TODO Could be improved (table, list...)*/
    QwtPlotCurve curveA;
    QwtPlotCurve curveB;
    QwtPlotCurve curveC;
    QwtPlotCurve curveD;
    
};

#endif
