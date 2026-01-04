/*
 * Copyright (c) 2011-2025 Meltytech, LLC
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

#ifndef MLTCONTROLLER_H
#define MLTCONTROLLER_H

#include "transportcontrol.h"

#include "settings.h"

#include <Mlt.h>
#include <QImage>
#include <QMutex>
#include <QScopedPointer>
#include <QString>
#include <QTemporaryFile>
#include <QUuid>

#define MLT_LC_CATEGORY LC_ALL
#define MLT_LC_NAME "LC_ALL"

#define kAudioIndexProperty "astream"
#define kVideoIndexProperty "vstream"

namespace Mlt {

const int kMaxImageDurationSecs = 3600 * 4;
extern const QString XmlMimeType;

class TransportControl : public TransportControllable
{
    Q_OBJECT
public slots:
    void play(double speed = 1.0) override;
    void pause(int position = -1) override;
    void stop() override;
    void seek(int position) override;
    void rewind(bool forceChangeDirection) override;
    void fastForward(bool forceChangeDirection) override;
    void previous(int currentPosition) override;
    void next(int currentPosition) override;
    void setIn(int) override;
    void setOut(int) override;
};

class Controller
{
protected:
    Controller();
    virtual int reconfigure(bool isMulti) = 0;

public:
    enum {
        FILTER_INDEX_ALL = -1,
        FILTER_INDEX_ENABLED = -2,
    };

    static Controller &singleton(QObject *parent = nullptr);
    virtual ~Controller();
    static void destroy();

    virtual QObject *videoWidget() = 0;
    virtual int setProducer(Mlt::Producer *, bool isMulti = false);
    virtual int open(const QString &url, const QString &urlToSave, bool skipConvert = false);
    bool openXML(const QString &filename);
    virtual void close();
    virtual int displayWidth() const = 0;
    virtual int displayHeight() const = 0;

    void closeConsumer();
    virtual void play(double speed = 1.0);
    bool isPaused() const;
    virtual void pause(int position = -1);
    void stop();
    bool enableJack(bool enable = true);
    void setVolume(double volume, bool muteOnPause = true);
    double volume() const;
    void onWindowResize();
    virtual void seek(int position);
    virtual void refreshConsumer(bool scrubAudio = false);
    bool saveXML(const QString &filename,
                 Service *service = nullptr,
                 bool withRelativePaths = true,
                 QTemporaryFile *tempFile = nullptr,
                 bool proxy = false,
                 QString projectNote = QString());
    QString XML(Service *service = nullptr, bool withProfile = false, bool withMetadata = true);
    int consumerChanged();
    void setProfile(const QString &profile_name);
    void setAudioChannels(int audioChannels);
    void setProcessingMode(ShotcutSettings::ProcessingMode mode);
    QString resource() const;
    bool isSeekable(Mlt::Producer *p = nullptr) const;
    int maxFrameCount() const;
    bool isLiveProducer(Mlt::Producer *p = nullptr) const;
    bool isClip() const;
    bool isClosedClip(Producer *producer = nullptr) const;
    bool isSeekableClip();
    bool isPlaylist() const;
    bool isMultitrack() const;
    bool isImageProducer(Service *service) const;
    bool isFileProducer(Service *service) const;
    static bool isProjectProducer(Service *service);
    void rewind(bool forceChangeDirection);
    void fastForward(bool forceChangeDirection);
    void previous(int currentPosition);
    void next(int currentPosition);
    void setIn(int);
    void setOut(int);
    void fixLengthProperties(Service &service);
    void reload(const QString &xml);
    void resetURL();
    QImage image(Frame *frame, int width, int height);
    QImage image(Mlt::Producer &producer, int frameNumber, int width, int height);
    void updateAvformatCaching(int trackCount);
    bool isAudioFilter(const QString &name);
    int realTime() const;
    void setImageDurationFromDefault(Service *service) const;
    void setDurationFromDefault(Producer *service) const;
    void lockCreationTime(Producer *producer) const;
    Producer *setupNewProducer(Producer *newProducer) const;
    QUuid uuid(Mlt::Properties &properties) const;
    void setUuid(Mlt::Properties &properties, QUuid uid) const;
    QUuid ensureHasUuid(Mlt::Properties &properties) const;
    static void copyFilters(Mlt::Producer &fromProducer,
                            Mlt::Producer &toProducer,
                            bool fromClipboard = false,
                            int filterIndex = FILTER_INDEX_ENABLED);
    void copyFilters(Mlt::Producer *producer = nullptr, int filterIndex = FILTER_INDEX_ENABLED);
    void pasteFilters(Mlt::Producer *producer = nullptr, Mlt::Producer *fromProducer = nullptr);
    static void adjustFilters(Mlt::Producer &producer, int startIndex = 0);
    static void adjustFilter(
        Mlt::Filter *filter, int in, int out, int inDelta, int outDelta, int keyframeDelta);
    static void adjustClipFilters(
        Mlt::Producer &producer, int in, int out, int inDelta, int outDelta, int keyframeDelta);
    bool hasFiltersOnClipboard() const
    {
        return m_filtersClipboard->is_valid() && m_filtersClipboard->filter_count() > 0;
    }
    QString filtersClipboardXML() { return XML(m_filtersClipboard.get()); }

    int audioChannels() const { return m_audioChannels; }
    ShotcutSettings::ProcessingMode processingMode() const { return m_processingMode; }
    Mlt::Repository *repository() const { return m_repo; }
    Mlt::Profile &profile() { return m_profile; }
    Mlt::Profile &previewProfile() { return m_previewProfile; }
    Mlt::Producer *producer() const { return m_producer.data(); }
    Mlt::Consumer *consumer() const { return m_consumer.data(); }
    const QString &URL() const { return m_url; }
    const TransportControllable *transportControl() const { return &m_transportControl; }
    Mlt::Producer *savedProducer() const { return m_savedProducer.data(); }
    void setSavedProducer(Mlt::Producer *producer);
    static Mlt::Filter *getFilter(const QString &name, Mlt::Service *service);
    static Mlt::Link *getLink(const QString &name, Mlt::Service *service);
    QString projectFolder() const { return m_projectFolder; }
    void setProjectFolder(const QString &folderName);
    QChar decimalPoint();
    static void resetLocale();
    static int filterIn(Mlt::Playlist &playlist, int clipIndex);
    static int filterOut(Mlt::Playlist &playlist, int clipIndex);
    void setPreviewScale(int scale);
    void updatePreviewProfile();
    static void purgeMemoryPool();
    static bool fullRange(Mlt::Producer &producer);
    static bool isMltXml(const QString &s) { return s.contains("<mlt "); }
    static bool isTrackProducer(Mlt::Producer &producer);
    static int checkFile(const QString &path);
    bool blockRefresh(bool block);
    void configureHardwareDecoder(bool enable);

    class RefreshBlocker
    {
    public:
        RefreshBlocker() { singleton().blockRefresh(true); }
        ~RefreshBlocker() { singleton().blockRefresh(false); }
    };

protected:
    Mlt::Repository *m_repo;
    QScopedPointer<Mlt::Producer> m_producer;
    QScopedPointer<Mlt::FilteredConsumer> m_consumer;

private:
    Mlt::Profile m_profile;
    Mlt::Profile m_previewProfile;
    int m_audioChannels{2};
    ShotcutSettings::ProcessingMode m_processingMode{ShotcutSettings::Native8Cpu};
    QScopedPointer<Mlt::Filter> m_jackFilter;
    QString m_url;
    double m_volume{1.0};
    TransportControl m_transportControl;
    QScopedPointer<Mlt::Producer> m_savedProducer;
    QScopedPointer<Mlt::Producer> m_filtersClipboard;
    unsigned m_skipJackEvents{0};
    QString m_projectFolder;
    QMutex m_saveXmlMutex;
    bool m_blockRefresh;

    static void on_jack_started(mlt_properties owner, void *object, mlt_event_data data);
    void onJackStarted(int position);
    static void on_jack_stopped(mlt_properties owner, void *object, mlt_event_data data);
    void onJackStopped(int position);
    void stopJack();
    void initFiltersClipboard();
};

} // namespace Mlt

#define MLT Mlt::Controller::singleton()

#endif // MLTCONTROLLER_H
