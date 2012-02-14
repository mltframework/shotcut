/*
 * Copyright (c) 2012 Meltytech, LLC
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

#ifndef PLAYER_H
#define PLAYER_H

#include <QWidget>
#include <QtGui/QIcon>
#include <QtCore/QSettings>

class ScrubBar;
class QSpinBox;
class QLabel;

namespace Mlt {
    class QFrame;
}
namespace Ui {
    class Player;
}

class Player : public QWidget
{
    Q_OBJECT
public:
    explicit Player(QWidget *parent = 0);

    void setIn(unsigned);
    void setOut(unsigned);
    unsigned position() const {
        return m_position;
    }

signals:
    void endOfStream();
    void seeked();
    void showStatusMessage(QString);

public slots:
    void play(double speed = 1.0);
    void pause();
    void togglePlayPaused();
    void seek(int position);
    void onProducerOpened();
    void onShowFrame(Mlt::QFrame frame, int position);

protected:
    void resizeEvent(QResizeEvent* event);

private:
    void readSettings();

    Ui::Player* ui;
    QSettings m_settings;
    ScrubBar* m_scrubber;
    QSpinBox* m_positionSpinner;
    QLabel* m_durationLabel;
    unsigned m_position;
    QIcon m_playIcon;
    QIcon m_pauseIcon;

private slots:
    void onInChanged(int in);
    void onOutChanged(int out);
    void on_actionSkipNext_triggered();
    void on_actionSkipPrevious_triggered();
    void on_actionRewind_triggered();
    void on_actionFastForward_triggered();
    void onVideoWidgetContextMenu(const QPoint& pos);
    void on_actionProgressive_triggered(bool checked);
    void on_actionLowQuality_triggered(bool checked);
    void on_actionMediumQuality_triggered(bool checked);
    void on_actionHighQuality_triggered(bool checked);
};

#endif // PLAYER_H
