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
 * @file comborange.cpp
 * @brief Definition of ComboRange class.
 * ComboRange is a ComboBox linked to a label.
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>

#include "comborange.h"

ComboRange::ComboRange(QWidget *parent)
    : QWidget(parent)
{
    init();
}

ComboRange::ComboRange(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    init();
    setText(text);
}

ComboRange::~ComboRange()
{
  delete label;
  delete combo;
}

void ComboRange::init()
{
    combo = new QComboBox();
    //combo->setSegmentStyle(ComboRange::Filled);

    label = new QLabel;
    label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    connect(combo, SIGNAL(currentIndexChanged(int)), this, SIGNAL(valueChanged(int)));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(combo);
    layout->addWidget(label);
    setLayout(layout);

}

int ComboRange::value() const
{
    return combo->currentIndex();
}

QString ComboRange::text() const
{
    return label->text();
}

void ComboRange::setValue(int index, const QString &text)
{
    (void)index;
    combo->addItem(text);
}

void ComboRange::setValues(const QStringList &list)
{
    combo->addItems(list);
}

void ComboRange::setText(const QString &text)
{
    label->setText(text);
}

void ComboRange::setCurrentIndex(int index)
{
    combo->setCurrentIndex(index);
}
