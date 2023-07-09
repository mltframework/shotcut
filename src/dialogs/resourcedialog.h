/*
 * Copyright (c) 2023 Meltytech, LLC
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

#ifndef RESOURCEDIALOG_H
#define RESOURCEDIALOG_H

#include <QDialog>

class ResourceWidget;

namespace Mlt {
class Producer;
}

class ResourceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ResourceDialog(QWidget *parent = 0);

    void search(Mlt::Producer *producer);
    void add(Mlt::Producer *producer);
    void selectTroubleClips();
    bool hasTroubleClips();

private slots:
    void convert();

protected:
    virtual void showEvent(QShowEvent *event) override;

private:
    ResourceWidget *m_resourceWidget;
};

#endif // RESOURCEDIALOG_H
