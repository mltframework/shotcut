/***************************************************************************
 *   Copyright (C) 2010 by Marco Gittler (g.marco@freenet.de)              *
 *   Copyright (C) 2012 by Dan Dennedy (dan@dennedy.org)                   *
 *   Copyright (C) 2015 Meltytech, LLC                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "audiosignal.h"
#include <QLabel>
#include <QPainter>

AudioSignal::AudioSignal(QWidget *parent): QWidget(parent)
{
    const QFont& font = QWidget::font();
    const int fontSize = font.pointSize() - (font.pointSize() > 10? 2 : (font.pointSize() > 8? 1 : 0));
    QWidget::setFont(QFont(font.family(), fontSize));
    setMinimumHeight(300);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMinimumWidth(fontMetrics().width("-60") + 20);
    dbscale << 0 << -5 << -10 << -15 << -20 << -25 << -30 << -35 << -40 << -50;
}

//----------------------------------------------------------------------------
// IEC standard dB scaling -- as borrowed from meterbridge (c) Steve Harris

static inline double IEC_Scale(double dB)
{
	double fScale = 1.0f;

	if (dB < -70.0f)
		fScale = 0.0f;
	else if (dB < -60.0f)
		fScale = (dB + 70.0f) * 0.0025f;
	else if (dB < -50.0f)
		fScale = (dB + 60.0f) * 0.005f + 0.025f;
	else if (dB < -40.0)
		fScale = (dB + 50.0f) * 0.0075f + 0.075f;
	else if (dB < -30.0f)
		fScale = (dB + 40.0f) * 0.015f + 0.15f;
	else if (dB < -20.0f)
		fScale = (dB + 30.0f) * 0.02f + 0.3f;
	else if (dB < -0.001f || dB > 0.001f)  /* if (dB < 0.0f) */
		fScale = (dB + 20.0f) * 0.025f + 0.5f;

	return fScale;
}

void AudioSignal::showAudio(const QVector<double>& dbLevels)
{
    if (peeks.size() != dbLevels.size()) {
        channels = QVector<double>(dbLevels.size(), 0);
        peeks = QVector<double>(dbLevels.size(), 0);
        peekage = QVector<double>(dbLevels.size(), 0);
    }
    for (int chan = 0; chan < dbLevels.size(); chan++)
    {
        // Scale all levels.
        channels[chan] = IEC_Scale(dbLevels[chan]);
        // Save peaks
        peekage[chan] = peekage[chan] + 1;
        if (peeks[chan] < channels[chan] || peekage[chan] > 50) {
            peekage[chan] = 0;
            peeks[chan] = channels[chan];
        }
    }
    update();
}

void AudioSignal::paintEvent(QPaintEvent* /*e*/)
{
    if (!isVisible())
        return;
    QPainter p(this);
    int numchan = channels.size();
    bool horiz = width() > height();
    int dbsize = 0;
    bool showdb = false;
    double h = 0;
    double w = 0;

    if (horiz) {
        dbsize = fontMetrics().height() + 2;
        showdb = height() > (dbsize + 2);
        h = height();
        w = IEC_Scale(-dbscale[0]) * width();

    } else {
        dbsize = fontMetrics().width("-60") + 2;
        showdb = width() > (dbsize + 2);
        h = IEC_Scale(-dbscale[0]) * height();
        w = width();
    }

    //valpixel=1.0 for 127, 1.0+(1/40) for 1 short oversample, 1.0+(2/40) for longer oversample
    for (int i = 0; i < numchan; i++) {
        int maxx = 0;
        int xdelta = 0;
        int y2 = 0;
        int y1 = 0;
        int x2 = 0;
        if (horiz) {
            maxx = w * channels[i];
            xdelta = w / 42;
            y2 = (showdb? height() - dbsize : height() ) / numchan - 1;
            y1 = (showdb? height() - dbsize : height() ) * i / numchan;
            x2 = maxx > xdelta ? xdelta - 1 : maxx - 1;
        } else {
            maxx =  h  * channels[i];
            xdelta = h / 42 ;
            y2 = (showdb? width() - dbsize : width()) / numchan - 1;
            y1 = (showdb? width() - dbsize : width()) *  i / numchan;
            x2 = maxx > xdelta ? xdelta - 3 : maxx - 3;
        }

        for (int segment = 0; segment <= 42; segment++) {
            int x1 = segment * xdelta;
            QColor sig = Qt::green;
            //value of actual painted digit
            double ival = (double) x1 / (double) xdelta / 42.0;
            if (ival > 40.0/42.0)
                sig = Qt::red;
            else if (ival > 37.0/42.0)
                sig = Qt::darkYellow;
            else if (ival > 30.0/42.0)
                sig = Qt::yellow;
            if (maxx > 0) {
                if (horiz)
                    p.fillRect(x1, y1, x2, y2, QBrush(sig, Qt::SolidPattern));
                else
                    p.fillRect(y1 + dbsize, height() - x1, y2,-x2, QBrush(sig, Qt::SolidPattern));
                maxx -= xdelta;
            }
        }
        int xp = peeks[i] * (horiz? w : h);
        p.fillRect(horiz? qMin(width() - 3, xp) : y1 + dbsize,
                   horiz? y1 : qMax(0, height() - xdelta - xp),
                   horiz? 3 : y2,
                   horiz? y2 : 3,
                   palette().text());
    }
    if (showdb) {
        //draw db value at related pixel
        double xf = 0;
        double prevXf = horiz ? width() : height();
        foreach (int i, dbscale) {
            QString dbLabel = QString().sprintf("%d", i);
            if (!horiz) {
                xf = IEC_Scale(i) * h;
                if (prevXf - xf > fontMetrics().height()) {
                    p.drawText(dbsize - fontMetrics().width(dbLabel), height() - xf, dbLabel);
                    prevXf = xf;
                }
            } else {
                xf = IEC_Scale(i) * w - fontMetrics().width(dbLabel) / 2;
                if (prevXf -xf > fontMetrics().width(dbLabel)) {
                    p.drawText(xf, height() - 2, dbLabel);
                    prevXf = xf;
                }
            }
        }
    }
    p.end();
}
