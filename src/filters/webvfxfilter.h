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

#ifndef WEBVFXFILTER_H
#define WEBVFXFILTER_H

#include <QWidget>
#include <mlt++/MltFilter.h>

namespace Ui {
class WebvfxFilter;
}

class WebvfxFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit WebvfxFilter(Mlt::Filter filter, QWidget *parent = 0);
    ~WebvfxFilter();
    
private slots:
    void on_openButton_clicked();
    void on_editButton_clicked();
    void onHtmlClosed();
    void on_reloadButton_clicked();

    void on_webvfxCheckBox_clicked(bool checked);
    
    void on_newButton_clicked();
    
private:
    Ui::WebvfxFilter *ui;
    Mlt::Filter m_filter;

    void setFilterFileName(QString filename);
};

#endif // WEBVFXFILTER_H
