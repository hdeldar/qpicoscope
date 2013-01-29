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
 * @brief Definition of Screen class.
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */


#include <QDateTime>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>

#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_canvas.h>

#include <math.h>
#include <stdlib.h>

#include "screen.h"

Screen::Screen(QWidget *parent)
    : QwtPlot(parent)
{
    initGradient();

    currentVoltCaliber = 0.;
    currentTimeCaliber = 0.;
    currentTrigger = E_TRIGGER_AUTO;
    currentCurrent = E_CURRENT_AC;

    setPalette(QPalette(QColor(250, 250, 200)));
    setAutoFillBackground(true);
    setAxisTitle(QwtPlot::xBottom, "Time [s]");
    setAxisScale(QwtPlot::xBottom, 0.0, 1.0);
    setAxisTitle(QwtPlot::yLeft, "Voltage [V]");
    setAxisScale(QwtPlot::yLeft,-5.0,5.0);
    setAutoReplot(false);

    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->setPen(QPen(Qt::gray, 0.0, Qt::DotLine));
    grid->setXAxis(0);
    grid->setYAxis(0);
    grid->enableX(true);
    grid->enableXMin(false);
    grid->enableY(true);
    grid->enableYMin(false);
    grid->attach(this);
   
    curveA.setStyle(QwtPlotCurve::Lines);
    curveA.setPen(QPen(Qt::green));
    curveA.setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curveA.setPaintAttribute(QwtPlotCurve::ClipPolygons, false);
    curveA.attach(this);
    curveB.setStyle(QwtPlotCurve::Lines);
    curveB.setPen(QPen(Qt::red));
    curveB.setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curveB.setPaintAttribute(QwtPlotCurve::ClipPolygons, false);
    curveB.attach(this);
    curveC.setStyle(QwtPlotCurve::Lines);
    curveC.attach(this);
    curveD.setStyle(QwtPlotCurve::Lines);
    curveD.attach(this);


}

void Screen::initGradient()
{
    QPalette pal = canvas()->palette();

    QLinearGradient gradient( 0.0, 0.0, 1.0, 0.0 );
    gradient.setCoordinateMode( QGradient::StretchToDeviceMode );
    gradient.setColorAt(0.0, QColor( 0, 49, 110 ) );
    gradient.setColorAt(1.0, QColor( 0, 87, 174 ) );

    pal.setBrush(QPalette::Window, QBrush(gradient));

    canvas()->setPalette(pal);
}

void Screen::setVoltCaliber(double voltCaliber)
{
    if (currentVoltCaliber == voltCaliber)
        return;
    currentVoltCaliber = voltCaliber;
    // update a part:
    //update(cannonRect());
    //emit voltCaliberChanged(currentVoltCaliber);
    setAxisScale(QwtPlot::yLeft,-(5*currentVoltCaliber),(5*currentVoltCaliber), currentVoltCaliber);
    // update all:
    //replot();
    update();
}

void Screen::setTimeCaliber(double timeCaliber)
{
    if (timeCaliber < 0)
        timeCaliber = 0;
    if (currentTimeCaliber == timeCaliber)
        return;
    currentTimeCaliber = timeCaliber;
    setAxisScale(QwtPlot::xBottom, 0.0, 5*currentTimeCaliber, currentTimeCaliber);
    // update all:
    //replot();
    update();
    //emit timeCaliberChanged(currentTimeCaliber);
}

void Screen::setCurrent(current_e current)
{
    if (currentCurrent == current)
        return;
    currentCurrent = current;
    // update all:
    update();
    //emit currentChanged(currentCurrent);
}

void Screen::setTrigger(trigger_e trigger)
{
    if (currentTrigger == trigger)
        return;
    currentTrigger = trigger;
    // update all:
    update();
    //emit triggerChanged(currentTimeCaliber);
}


//! [2]
void Screen::mousePressEvent(QMouseEvent *event)
{
    (void)event; //avoid warning for now
//    if (event->button() != Qt::LeftButton)
//        return;
//    if (barrelHit(event->pos()))
//        barrelPressed = true;
}
//! [2]

//! [3]
void Screen::mouseMoveEvent(QMouseEvent *event)
{
      (void)event; //avoid warning for now
//    if (!barrelPressed)
//        return;
//    QPoint pos = event->pos();
//    if (pos.x() <= 0)
//        pos.setX(1);
//    if (pos.y() >= height())
//        pos.setY(height() - 1);
//    double rad = atan(((double)rect().bottom() - pos.y()) / pos.x());
//    setAngle(qRound(rad * 180 / 3.14159265));
//! [3] //! [4]
}
//! [4]

//! [5]
void Screen::mouseReleaseEvent(QMouseEvent *event)
{
    (void)event;
//    if (event->button() == Qt::LeftButton)
//        barrelPressed = false;
}
//! [5]

void Screen::paintEvent(QPaintEvent *event)
{

    (void)event;
//    QPainter painter(this);
//    painter.drawText(200, 140,
//                     tr("Volt/div = ") + QString::number(currentVoltCaliber));
//    painter.drawText(200, 160,
//                     tr("time/div = ") + QString::number(currentTimeCaliber));
//    painter.drawText(200, 180,
//                     tr("current = ") + QString::number(currentCurrent));
//    painter.drawText(200, 200,
//                     tr("trigger = ") + QString::number(currentTrigger));

//    QPainter painter(this);

//    if (gameEnded) {
//        painter.setPen(Qt::black);
//        painter.setFont(QFont("Courier", 48, QFont::Bold));
//        painter.drawText(rect(), Qt::AlignCenter, tr("Game Over"));
//    }
//    paintCannon(painter);
////! [6]
//    paintBarrier(painter);
////! [6]
//    if (isShooting())
//        paintShot(painter);
//    if (!gameEnded)
//        paintTarget(painter);
    // TODO calling replot here is freezing the mainwindow.... But not calling it will never show the curves...
    replot();
}

//void Screen::paintShot(QPainter &painter)
//{
//    painter.setPen(Qt::NoPen);
//    painter.setBrush(Qt::black);
//    painter.drawRect(shotRect());
//}

//void Screen::paintTarget(QPainter &painter)
//{
//    painter.setPen(Qt::black);
//    painter.setBrush(Qt::red);
//    painter.drawRect(targetRect());
//}

////! [7]
//void Screen::paintBarrier(QPainter &painter)
//{
//    painter.setPen(Qt::black);
//    painter.setBrush(Qt::yellow);
//    painter.drawRect(barrierRect());
//}
//! [7]

//const QRect barrelRect(30, -5, 20, 10);

//QRect Screen::cannonRect() const
//{
//    QRect result(0, 0, 50, 50);
//    result.moveBottomLeft(rect().bottomLeft());
//    return result;
//}

//QRect Screen::shotRect() const
//{
//    const double gravity = 4;

//    double time = timerCount / 20.0;
//    double velocity = shootForce;
//    double radians = shootAngle * 3.14159265 / 180;

//    double velx = velocity * cos(radians);
//    double vely = velocity * sin(radians);
//    double x0 = (barrelRect.right() + 5) * cos(radians);
//    double y0 = (barrelRect.right() + 5) * sin(radians);
//    double x = x0 + velx * time;
//    double y = y0 + vely * time - 0.5 * gravity * time * time;

//    QRect result(0, 0, 6, 6);
//    result.moveCenter(QPoint(qRound(x), height() - 1 - qRound(y)));
//    return result;
//}

//QRect Screen::targetRect() const
//{
//    QRect result(0, 0, 20, 10);
//    result.moveCenter(QPoint(target.x(), height() - 1 - target.y()));
//    return result;
//}

//! [8]
//QRect Screen::barrierRect() const
//{
//    return QRect(145, height() - 100, 15, 99);
//}
////! [8]

////! [9]
//bool Screen::barrelHit(const QPoint &pos) const
//{
//    QMatrix matrix;
//    matrix.translate(0, height());
//    matrix.rotate(-currentAngle);
//    matrix = matrix.inverted();
//    return barrelRect.contains(matrix.map(pos));
//}
////! [9]

//bool Screen::isShooting() const
//{
//    return autoShootTimer->isActive();
//}

//QSize Screen::sizeHint() const
//{
//    return QSize(400, 300);
//}
 

int8_t Screen::setData(uint8_t channel_id, double *x_data, double *y_data, uint32_t nb_points)
{

    QwtPlotCurve *curve = NULL;
    // select channel_id
    switch(channel_id)
    {
        case 1:
            curve = &curveA;
        break;
        case 2:
            curve = &curveB;
        break;
        case 3:
            curve = &curveC;
        break;
        case 4:
            curve = &curveD;
        break;
        default:
            ERROR("invalid channel id : %d\n", channel_id);
            return -1;

    }
    
    if( nb_points <= INT_MAX )
    {
#if ( QWT_VERSION >= 0x060000)
        curve->setSamples( x_data, y_data, (int)nb_points);
#else
        curve->setData( x_data, y_data, (int)nb_points);
#endif
        
    }
    else
    {
        // crazy to get such an amount of data at once...
#if ( QWT_VERSION >= 0x060000)
        curve->setSamples( x_data, y_data, INT_MAX);
        curve->setSamples( x_data + INT_MAX, y_data + INT_MAX, (int)(nb_points-INT_MAX));
#else
        curve->setData( x_data, y_data, INT_MAX);
        curve->setData( x_data + INT_MAX, y_data + INT_MAX, (int)(nb_points-INT_MAX));
#endif
    }
    update();
    return 0;
}

