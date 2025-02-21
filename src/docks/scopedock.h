/*
 * Copyright (c) 2015 Meltytech, LLC
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

#ifndef SCOPEDOCK_H
#define SCOPEDOCK_H

#include "widgets/scopes/scopewidget.h"

#include <QDockWidget>
#include <QObject>

class ScopeController;

class ScopeDock Q_DECL_FINAL : public QDockWidget
{
    Q_OBJECT

public:
    explicit ScopeDock(ScopeController *scopeController, ScopeWidget *scopeWidget);

protected:
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;

private:
    ScopeController *m_scopeController;
    ScopeWidget *m_scopeWidget;

    void setWidget(QWidget *widget); // Private to disallow use

private slots:
    void onActionToggled(bool checked);
};

#endif // SCOPEDOCK_H
