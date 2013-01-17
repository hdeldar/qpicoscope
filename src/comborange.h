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
 * @file comborange.h
 * @brief Declaration of ComboRange class.
 * ComboRange is a ComboBox linked to a label.
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */

#ifndef COMBORANGE_H
#define COMBORANGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
QT_END_NAMESPACE

class ComboRange : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief constructor
     * @param[in] parent widget pointer
     */
    ComboRange(QWidget *parent = 0);
    /**
     * @brief constructor
     * @param[in] label text under the ComboBox
     * @param[in] parent widget pointer
     */
    ComboRange(const QString &text, QWidget *parent = 0);
    /**
     * @brief destructor
     */
    virtual ~ComboRange();
    /**
     * @brief get current ComboRange's value
     * @return ComboBox's current index
     */
    int value() const;
    /**
     * @brief get current ComboRange's label text
     * @return Label's current text
     */
    QString text() const;
    /**
     * @brief set current ComboRange's label text
     @param[in] Label's text to set
     */
    void setText(const QString &text);
    /**
     * @brief set ComboBox text at index
     * @param[in]: ComboBox index at which text should be inserted
     * @param[in]: ComboBox text to add
     */
    void setValue(int index, const QString & text);
    /**
     * @brief set ComboBox text
     * @param[in]: A list of string that should be inserted within the ComboBox
     */
    void setValues(const QStringList & list);

public slots:


signals:
    void valueChanged(int newValue);

private:
    void init();
    QComboBox *combo;
    QLabel *label;
};

#endif
