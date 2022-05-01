/*
 * Copyright (c) 2015-2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#ifndef SCOPECONTROLLER_H
#define SCOPECONTROLLER_H

#include <QObject>
#include <QString>
#include "sharedframe.h"

class QMainWindow;
class QMenu;
class QWidget;

class ScopeController Q_DECL_FINAL : public QObject
{
    Q_OBJECT

public:
    ScopeController(QMainWindow *mainWindow, QMenu *menu);

signals:
    void newFrame(const SharedFrame &frame);

private:
    template<typename ScopeTYPE> void createScopeDock(QMainWindow *mainWindow, QMenu *menu);

};

#endif // SCOPECONTROLLER_H
