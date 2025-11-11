/*
 * Copyright (c) 2025 Meltytech, LLC
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

#ifndef SCREENCAPTURETOOLBAR_H
#define SCREENCAPTURETOOLBAR_H

#include <QCheckBox>
#include <QPushButton>
#include <QWidget>

class ScreenCapture;

class ScreenCaptureToolbar : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenCaptureToolbar(bool isRecordingMode, QWidget *parent = nullptr);
    ~ScreenCaptureToolbar();

signals:
    void captureModeSelected(int mode, bool minimizeShotcut, bool recordAudio);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onFullscreenClicked();
    void onRectangleClicked();
    void onWindowClicked();

private:
    bool m_isRecordingMode;
    QPushButton *m_fullscreenButton;
    QPushButton *m_rectangleButton;
    QPushButton *m_windowButton;
    QCheckBox *m_minimizeCheckbox;
    QCheckBox *m_audioCheckbox;
};

#endif // TOOLBARWIDGET_H
