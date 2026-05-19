/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include <private/qplatformmediaplugin_p.h>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

// Minimal QPlatformVideoSink — the base class does all the work
class MinimalVideoSink : public QPlatformVideoSink
{
    Q_OBJECT
public:
    explicit MinimalVideoSink(QVideoSink *sink)
        : QPlatformVideoSink(sink)
    {}
};

// Minimal integration — only creates video sinks
class MinimalMediaIntegration : public QPlatformMediaIntegration
{
public:
    MinimalMediaIntegration()
        : QPlatformMediaIntegration(QLatin1String("minimal"))
    {}

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    q23::expected<QPlatformVideoSink *, QString> createVideoSink(QVideoSink *sink) override
    {
        return new MinimalVideoSink(sink);
    }
#else
    QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *sink) override
    {
        return new MinimalVideoSink(sink);
    }
#endif
};

// Plugin entry point
class MinimalMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "minimalmediaplugin.json")

public:
    MinimalMediaPlugin(QObject *parent = nullptr)
        : QPlatformMediaPlugin(parent)
    {}

    QPlatformMediaIntegration *create(const QString &key) override
    {
        if (key == QLatin1String("minimal"))
            return new MinimalMediaIntegration;
        return nullptr;
    }
};

QT_END_NAMESPACE

#include "minimalmediaplugin.moc"
