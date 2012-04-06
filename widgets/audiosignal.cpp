/***************************************************************************
 *   Copyright (C) 2010 by Marco Gittler (g.marco@freenet.de)              *
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

#include <QVBoxLayout>
#include <QLabel>
#include <QAction>
#include <QPainter>
#include <QDebug>
#include <QList>

#include <math.h>

AudioSignal::AudioSignal(QWidget *parent): QWidget(parent)
{
    //QVBoxLayout *vbox=new QVBoxLayout(this);
    //label=new QLabel();
    //vbox->addWidget(label);
    setMinimumHeight(10);
    setMinimumWidth(61);
    dbscale << 0 << -1 << -2 << -3 << -4 << -5 << -6 << -8 << -10 << -20 << -40 ;
    setContextMenuPolicy(Qt::ActionsContextMenu);
    m_aMonitoringEnabled = new QAction(tr("Monitor audio signal"), this);
    m_aMonitoringEnabled->setCheckable(true);
    m_aMonitoringEnabled->setChecked(true);
    connect(m_aMonitoringEnabled, SIGNAL(toggled(bool)), this, SLOT(slotSwitchAudioMonitoring(bool)));
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(slotNoAudioTimeout()));
    addAction(m_aMonitoringEnabled);
}

AudioSignal::~AudioSignal()
{
    delete m_aMonitoringEnabled;
}

bool AudioSignal::monitoringEnabled() const
{
    return m_aMonitoringEnabled->isChecked();
}

void AudioSignal::slotReceiveAudio(const QVector<int16_t>& data, int, int num_channels, int samples)
{

    int num_samples = samples > 200 ? 200 : samples;

    QByteArray channels;
    int num_oversample=0;
    for (int i = 0; i < num_channels; i++) {
        long val = 0;
        double over1=0.0;
        double over2=0.0;
        for (int s = 0; s < num_samples; s ++) {
            int sample=abs(data[i+s*num_channels] / 128);
            val += sample;
            if (sample==128){
                num_oversample++;
            }else{
                num_oversample=0;
            }
            //if 3 samples over max => 1 peak over 0 db (0db=40.0)
            if (num_oversample>3){
                over1=41.0/42.0*127;
            }
            // 10 samples @max => show max signal
            if (num_oversample>10){
                over2=127;
            }



        }
        //max amplitude = 40/42, 3to10  oversamples=41, more then 10 oversamples=42 
        if (over2>0.0){
            channels.append(over2);		
        } else if (over1>0.0){
            channels.append(over1);		
        }else
            channels.append(val / num_samples*40.0/42.0);
    }
    showAudio(channels);
    m_timer.start(1000);
}

void AudioSignal::slotNoAudioTimeout(){
    peeks.fill(0);
    showAudio(QByteArray(2,0));
    m_timer.stop();
}

void AudioSignal::showAudio(const QByteArray arr)
{
    channels = arr;
    if (peeks.count()!=channels.count()){
        peeks=QByteArray(channels.count(),0);
        peekage=QByteArray(channels.count(),0);
    }
    for (int chan=0;chan<peeks.count();chan++)
    {
        peekage[chan]=peekage[chan]+1;
        if (  peeks.at(chan)<arr.at(chan) ||  peekage.at(chan)>50 )
        {
            peekage[chan]=0;
            peeks[chan]=arr[chan];
        }
    }
    update();
}

double AudioSignal::valueToPixel(double in)
{
	//in=0 -> return 0 (null length from max), in=127/127 return 1 (max length )
	return 1.0- log10( in)/log10(1.0/127.0);
}

void AudioSignal::paintEvent(QPaintEvent* /*e*/)
{
    if (!m_aMonitoringEnabled->isChecked()) {
        return;
    }
    QPainter p(this);
    int numchan = channels.size();
    bool horiz=width() > height();
    int dbsize=20;
    bool showdb=width()>(dbsize+40);
    //valpixel=1.0 for 127, 1.0+(1/40) for 1 short oversample, 1.0+(2/40) for longer oversample
    for (int i = 0; i < numchan; i++) {
        //int maxx= (unsigned char)channels[i] * (horiz ? width() : height() ) / 127;
        double valpixel=valueToPixel((double)(unsigned char)channels[i]/127.0);
        int maxx=  height()  * valpixel; 
        int xdelta= height()   /42 ;
        int _y2= (showdb?width()-dbsize:width () ) / numchan - 1  ;
        int _y1= (showdb?width()-dbsize:width() ) *i/numchan;
        int _x2= maxx >  xdelta ? xdelta - 3 : maxx - 3 ;
        if (horiz){
            dbsize=9;
            showdb=height()>(dbsize);
            maxx=width()*valpixel; 
            xdelta = width() / 42;
            _y2=( showdb?height()-dbsize:height() ) / numchan - 1 ;
            _y1= (showdb?height()-dbsize:height() ) * i/numchan;
            _x2= maxx >  xdelta ? xdelta - 1 : maxx - 1;
        }

        for (int x = 0; x <= 42; x++) {
            int _x1= x *xdelta;
            QColor sig=Qt::green;
            //value of actual painted digit
            double ival=(double)_x1/(double)xdelta/42.0;
            if (ival > 40.0/42.0){
                sig=Qt::red;
            }else if ( ival > 37.0/42.0){
                sig=Qt::darkYellow;
            }else if ( ival >30.0/42.0){
                sig=Qt::yellow;
            }
            if (maxx > 0) {
                if (horiz){
                    p.fillRect(_x1, _y1, _x2, _y2, QBrush(sig, Qt::SolidPattern) );
                }else{
                    p.fillRect(_y1, height()-_x1, _y2,-_x2, QBrush(sig, Qt::SolidPattern) );
                }
                maxx -= xdelta;
            }
        }
        int xp=valueToPixel((double)peeks.at(i)/127.0)*(horiz?width():height())-2;
        p.fillRect(horiz?xp:_y1,horiz?_y1:height()-xdelta-xp,horiz?3:_y2,horiz?_y2:3,QBrush(Qt::black,Qt::SolidPattern));

    }
    if (showdb){
        //draw db value at related pixel
        for (int l=0;l<dbscale.size();l++){
            if (!horiz){
                double xf=pow(10.0,(double)dbscale.at(l) / 20.0 )*(double)height();
                p.drawText(width()-20,height()-xf*40.0/42.0+20, QString().sprintf("%d",dbscale.at(l)));
            }else{
                double xf=pow(10.0,(double)dbscale.at(l) / 20.0 )*(double)width();
                p.drawText(xf*40/42-10,height()-2, QString().sprintf("%d",dbscale.at(l)));
            }
        }
    }
    p.end();
}

void AudioSignal::slotSwitchAudioMonitoring(bool)
{
    emit updateAudioMonitoring();
}
