/*
 * Copyright (c) 2014-2025 Meltytech, LLC
 * Inspiration: KDENLIVE colorpickerwidget.cpp by Till Theato (root@ttill.de)
 * Inspiration: QColorDialog.cpp
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

#include "colorpickeritem.h"
#include "Logger.h"

#include <QApplication>
#include <QGuiApplication>
#include <QImage>
#include <QScreen>
#include <QTimer>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif

ColorPickerItem::ColorPickerItem(QObject *parent)
    : QObject(parent)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    qDBusRegisterMetaType<QColor>();
#endif

    connect(this, &ColorPickerItem::pickColor, &m_selector, &ScreenSelector::startSelection);
    connect(&m_selector, &ScreenSelector::screenSelected, this, &ColorPickerItem::screenSelected);
    connect(&m_selector, &ScreenSelector::cancelled, this, &ColorPickerItem::cancelled);
}

void ColorPickerItem::screenSelected(const QRect &rect)
{
    m_selectedRect = rect;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (m_selector.useDBus())
        QTimer::singleShot(0, this, &ColorPickerItem::grabColorDBus);
    else
#endif
        // Give the frame buffer time to clear the selector window before
        // grabbing the color.
        QTimer::singleShot(200, this, &ColorPickerItem::grabColor);
}

void ColorPickerItem::grabColor()
{
    QScreen *screen = QGuiApplication::screenAt(m_selectedRect.topLeft());
    QPixmap screenGrab = screen->grabWindow(0,
                                            m_selectedRect.x() - screen->geometry().x(),
                                            m_selectedRect.y() - screen->geometry().y(),
                                            m_selectedRect.width(),
                                            m_selectedRect.height());
    QImage image = screenGrab.toImage();
    int numPixel = qMax(image.width() * image.height(), 1);
    int sumR = 0;
    int sumG = 0;
    int sumB = 0;

    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            QColor color = image.pixel(x, y);
            sumR += color.red();
            sumG += color.green();
            sumB += color.blue();
        }
    }

    QColor avgColor(sumR / numPixel, sumG / numPixel, sumB / numPixel);
    emit colorPicked(avgColor);
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)

QDBusArgument &operator<<(QDBusArgument &arg, const QColor &color)
{
    arg.beginStructure();
    arg << color.redF() << color.greenF() << color.blueF();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QColor &color)
{
    double red, green, blue;
    arg.beginStructure();
    arg >> red >> green >> blue;
    color.setRedF(red);
    color.setGreenF(green);
    color.setBlueF(blue);
    arg.endStructure();

    return arg;
}

void ColorPickerItem::grabColorDBus()
{
    QDBusMessage message
        = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.portal.Desktop"),
                                         QLatin1String("/org/freedesktop/portal/desktop"),
                                         QLatin1String("org.freedesktop.portal.Screenshot"),
                                         QLatin1String("PickColor"));
    message << QLatin1String("x11:") << QVariantMap{};
    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, &QDBusPendingCallWatcher::finished, [=](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            LOG_WARNING() << "Unable to get DBus reply: " << reply.error().message();
        } else {
            QDBusConnection::sessionBus().connect(QString(),
                                                  reply.value().path(),
                                                  QLatin1String("org.freedesktop.portal.Request"),
                                                  QLatin1String("Response"),
                                                  this,
                                                  SLOT(gotColorResponse(uint, QVariantMap)));
        }
    });
}

void ColorPickerItem::gotColorResponse(uint response, const QVariantMap &results)
{
    if (!response) {
        if (results.contains(QLatin1String("color"))) {
            const QColor color = qdbus_cast<QColor>(results.value(QLatin1String("color")));
            LOG_DEBUG() << "picked" << color;
            emit colorPicked(color);
        }
    } else {
        LOG_WARNING() << "Failed to grab screen" << response << results;
    }
}

#endif
