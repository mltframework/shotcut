/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NORMALIZE_H
#define NORMALIZE_H

#include <QWidget>
#include <MltFilter.h>

namespace Ui {
class Normalize;
}

class MeltJob;

class NormalizeFilter : public QWidget
{
    Q_OBJECT

public:
    explicit NormalizeFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~NormalizeFilter();

private slots:
    void on_analyzeButton_clicked();
    void onJobFinished(MeltJob* job, bool isSuccess);
    void on_buttonGroup_buttonClicked(int);
    void on_levelSpinBox_valueChanged(double value);

private:
    Ui::Normalize *ui;
    Mlt::Filter m_filter;
};

#endif // NORMALIZE_H
