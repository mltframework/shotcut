/*
 * Copyright (c) 2016-2022 Meltytech, LLC
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

#include "keyframesdock.h"

#include "models/attachedfiltersmodel.h"
#include "qmltypes/qmlproducer.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlview.h"
#include "widgets/docktoolbar.h"
#include "mainwindow.h"
#include "settings.h"
#include <Logger.h>

#include <QAction>
#include <QDir>
#include <QIcon>
#include <QMenu>
#include <QUrl>
#include <QVBoxLayout>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSlider>
#include <QToolButton>

#include <cmath>

static QmlMetadata m_emptyQmlMetadata;
static QmlFilter m_emptyQmlFilter;

KeyframesDock::KeyframesDock(QmlProducer *qmlProducer, QWidget *parent)
    : QDockWidget(tr("Keyframes"), parent)
    , m_qview(QmlUtilities::sharedEngine(), this)
    , m_qmlProducer(qmlProducer)
{
    LOG_DEBUG() << "begin";
    setObjectName("KeyframesDock");
    QIcon icon = QIcon::fromTheme("chronometer", QIcon(":/icons/oxygen/32x32/actions/chronometer.png"));
    setWindowIcon(icon);
    toggleViewAction()->setIcon(windowIcon());
    setMinimumSize(200, 50);

    setupActions();

    m_mainMenu = new QMenu(tr("Keyframes Main Menu"), this);
    m_mainMenu->addAction(m_actions["keyframesTrimInAction"]);
    m_mainMenu->addAction(m_actions["keyframesTrimOutAction"]);
    m_mainMenu->addAction(m_actions["keyframesAnimateInAction"]);
    m_mainMenu->addAction(m_actions["keyframesAnimateOutAction"]);
    m_mainMenu->addAction(m_actions["keyframesToggleKeyframeAction"]);
    m_mainMenu->addAction(m_actions["keyframesSeekPreviousSimpleAction"]);
    m_mainMenu->addAction(m_actions["keyframesSeekNextSimpleAction"]);
    QMenu *viewMenu = new QMenu(tr("View"), this);
    viewMenu->addAction(m_actions["keyframesZoomOutAction"]);
    viewMenu->addAction(m_actions["keyframesZoomInAction"]);
    viewMenu->addAction(m_actions["keyframesZoomFitAction"]);
    m_mainMenu->addMenu(viewMenu);

    m_keyMenu = new QMenu(tr("Keyframes Context Menu"), this);
    QMenu *keyTypeMenu = new QMenu(tr("Keyframe Type"), this);
    keyTypeMenu->addAction(m_actions["keyframesTypeHoldAction"]);
    keyTypeMenu->addAction(m_actions["keyframesTypeLinearAction"]);
    keyTypeMenu->addAction(m_actions["keyframesTypeSmoothAction"]);
    m_keyMenu->addMenu(keyTypeMenu);
    m_keyMenu->addAction(m_actions["keyframesRemoveAction"]);

    m_clipMenu = new QMenu(tr("Keyframes Clip Menu"), this);
    m_clipMenu->addAction(m_actions["keyframesRebuildAudioWaveformAction"]);

    QVBoxLayout *vboxLayout = new QVBoxLayout();
    vboxLayout->setSpacing(0);
    vboxLayout->setContentsMargins(0, 0, 0, 0);

    DockToolBar *toolbar = new DockToolBar(tr("Timeline Controls"));
    QToolButton *menuButton = new QToolButton();
    menuButton->setIcon(QIcon::fromTheme("show-menu",
                                         QIcon(":/icons/oxygen/32x32/actions/show-menu.png")));
    menuButton->setToolTip(tr("Timeline Menu"));
    menuButton->setAutoRaise(true);
    menuButton->setPopupMode(QToolButton::QToolButton::InstantPopup);
    menuButton->setMenu(m_mainMenu);
    toolbar->addWidget(menuButton);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["keyframesTrimInAction"]);
    toolbar->addAction(m_actions["keyframesTrimOutAction"]);
    toolbar->addAction(m_actions["keyframesAnimateInAction"]);
    toolbar->addAction(m_actions["keyframesAnimateOutAction"]);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["keyframesZoomOutAction"]);
    QSlider *zoomSlider = new QSlider();
    zoomSlider->setOrientation(Qt::Horizontal);
    zoomSlider->setMaximumWidth(200);
    zoomSlider->setMinimum(0);
    zoomSlider->setMaximum(300);
    zoomSlider->setValue(100);
    connect(zoomSlider, &QSlider::valueChanged, this, [&](int value) {
        if (!isVisible() || !m_qview.rootObject()) return;
        setZoom(value / 100.0);
    });
    connect(this, &KeyframesDock::timeScaleChanged, zoomSlider, [ = ]() {
        double value = 1.0;
        if (m_qview.rootObject()) {
            double scaleFactor = m_qview.rootObject()->property("timeScale").toDouble();
            double value = round(pow(scaleFactor - 0.01, 1.0 / 3.0) * 100.0);
            zoomSlider->setValue(value);
        }
    });
    toolbar->addWidget(zoomSlider);
    toolbar->addAction(m_actions["keyframesZoomInAction"]);
    toolbar->addAction(m_actions["keyframesZoomFitAction"]);

    vboxLayout->setMenuBar(toolbar);

    m_qview.setFocusPolicy(Qt::StrongFocus);
    m_qview.quickWindow()->setPersistentSceneGraph(false);
#ifndef Q_OS_MAC
    m_qview.setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    setWidget(&m_qview);

    QmlUtilities::setCommonProperties(m_qview.rootContext());
    m_qview.rootContext()->setContextProperty("keyframes", this);
    m_qview.rootContext()->setContextProperty("view", new QmlView(&m_qview));
    m_qview.rootContext()->setContextProperty("parameters", &m_model);
    setCurrentFilter(0, 0);
    connect(m_qview.quickWindow(), SIGNAL(sceneGraphInitialized()), SLOT(load()));

    vboxLayout->addWidget(&m_qview);
    QWidget *dockContentsWidget = new QWidget();
    dockContentsWidget->setLayout(vboxLayout);
    QDockWidget::setWidget(dockContentsWidget);

    LOG_DEBUG() << "end";
}

void KeyframesDock::setupActions()
{
    QIcon icon;
    QAction *action;

    action = new QAction(tr("Set Filter Start"), this);
    action->setObjectName("keyframesTrimInAction");
    action->setShortcut(QKeySequence(Qt::Key_BracketLeft));
    icon = QIcon::fromTheme("keyframes-filter-in",
                            QIcon(":/icons/oxygen/32x32/actions/keyframes-filter-in.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (m_qmlProducer && m_filter && m_filter->allowTrim()) {
            int i = m_qmlProducer->position() + m_qmlProducer->in();
            m_model.trimFilterIn(i);
        }
    });
    connect(this, &KeyframesDock::newFilter, action, [ = ]() {
        bool enabled = false;
        if (m_filter && m_filter->allowTrim())
            enabled = true;
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Set Filter End"), this);
    action->setObjectName("keyframesTrimOutAction");
    action->setShortcut(QKeySequence(Qt::Key_BracketRight));
    icon = QIcon::fromTheme("keyframes-filter-out",
                            QIcon(":/icons/oxygen/32x32/actions/keyframes-filter-out.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (m_qmlProducer && m_filter && m_filter->allowTrim()) {
            int i = m_qmlProducer->position() + m_qmlProducer->in();
            m_model.trimFilterOut(i);
        }
    });
    connect(this, &KeyframesDock::newFilter, action, [ = ]() {
        bool enabled = false;
        if (m_filter && m_filter->allowTrim())
            enabled = true;
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Set First Simple Keyframe"), this);
    action->setObjectName("keyframesAnimateInAction");
    action->setShortcut(QKeySequence(Qt::Key_BraceLeft));
    icon = QIcon::fromTheme("keyframes-simple-in",
                            QIcon(":/icons/oxygen/32x32/actions/keyframes-simple-in.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (m_qmlProducer && m_filter && m_filter->allowAnimateIn()) {
            int i = m_qmlProducer->position() + m_qmlProducer->in() - m_filter->in();
            m_filter->setAnimateIn(i);
        }
    });
    connect(this, &KeyframesDock::newFilter, action, [ = ]() {
        bool enabled = false;
        if (m_filter && m_filter->allowAnimateIn())
            enabled = true;
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Set Second Simple Keyframe"), this);
    action->setObjectName("keyframesAnimateOutAction");
    action->setShortcut(QKeySequence(Qt::Key_BraceRight));
    icon = QIcon::fromTheme("keyframes-simple-out",
                            QIcon(":/icons/oxygen/32x32/actions/keyframes-simple-out.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (m_qmlProducer && m_filter && m_filter->allowAnimateOut()) {
            int i = m_qmlProducer->position() + m_qmlProducer->in() - m_filter->in();
            m_filter->setAnimateOut(i);
        }
    });
    connect(this, &KeyframesDock::newFilter, action, [ = ]() {
        bool enabled = false;
        if (m_filter && m_filter->allowAnimateOut())
            enabled = true;
        action->setEnabled(enabled);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Zoom Keyframes Out"), this);
    action->setObjectName("keyframesZoomOutAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Minus));
    icon = QIcon::fromTheme("zoom-out",
                            QIcon(":/icons/oxygen/32x32/actions/zoom-out.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_qview.rootObject()) return;
        zoomOut();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Zoom Keyframes In"), this);
    action->setObjectName("keyframesZoomInAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Plus));
    icon = QIcon::fromTheme("zoom-in",
                            QIcon(":/icons/oxygen/32x32/actions/zoom-in.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_qview.rootObject()) return;
        zoomIn();
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Zoom Keyframes To Fit"), this);
    action->setObjectName("keyframesZoomFitAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_0));
    icon = QIcon::fromTheme("zoom-fit-best",
                            QIcon(":/icons/oxygen/32x32/actions/zoom-fit-best.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_qview.rootObject()) return;
        zoomToFit();
    });
    m_actions[action->objectName()] = action;

    QActionGroup *keyframeTypeActionGroup = new QActionGroup(this);
    keyframeTypeActionGroup->setExclusive(true);

    action = new QAction(tr("Hold"), this);
    action->setObjectName("keyframesTypeHoldAction");
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_qview.rootObject()) return;
        int currentTrack = m_qview.rootObject()->property("currentTrack").toInt();
        for (auto keyframeIndex : m_qview.rootObject()->property("selection").toList()) {
            m_model.setInterpolation(currentTrack, keyframeIndex.toInt(),
                                     KeyframesModel::DiscreteInterpolation);
        }
    });
    keyframeTypeActionGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Linear"), this);
    action->setObjectName("keyframesTypeLinearAction");
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_qview.rootObject()) return;
        int currentTrack = m_qview.rootObject()->property("currentTrack").toInt();
        for (auto keyframeIndex : m_qview.rootObject()->property("selection").toList()) {
            m_model.setInterpolation(currentTrack, keyframeIndex.toInt(), KeyframesModel::LinearInterpolation);
        }
    });
    keyframeTypeActionGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Smooth"), this);
    action->setObjectName("keyframesTypeSmoothAction");
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_qview.rootObject()) return;
        int currentTrack = m_qview.rootObject()->property("currentTrack").toInt();
        for (auto keyframeIndex : m_qview.rootObject()->property("selection").toList()) {
            m_model.setInterpolation(currentTrack, keyframeIndex.toInt(), KeyframesModel::SmoothInterpolation);
        }
    });
    keyframeTypeActionGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Remove"), this);
    action->setObjectName("keyframesRemoveAction");
    connect(action, &QAction::triggered, this, [&]() {
        if (!isVisible() || !m_qview.rootObject()) return;
        int currentTrack = m_qview.rootObject()->property("currentTrack").toInt();
        for (auto keyframeIndex : m_qview.rootObject()->property("selection").toList()) {
            m_model.remove(currentTrack, keyframeIndex.toInt());
        }
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Rebuild Audio Waveform"), this);
    action->setObjectName("keyframesRebuildAudioWaveformAction");
    action->setEnabled(Settings.timelineShowWaveforms());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (m_qmlProducer && Settings.timelineShowWaveforms()) {
            m_qmlProducer->remakeAudioLevels();
        }
    });
    connect(&Settings, &ShotcutSettings::timelineShowWaveformsChanged, action, [ = ]() {
        action->setEnabled(Settings.timelineShowWaveforms());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Seek Previous Simple Keyframe"), this);
    action->setObjectName("keyframesSeekPreviousSimpleAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_BracketLeft));
    action->setEnabled(m_qmlProducer && m_filter);
    connect(action, &QAction::triggered, this, [&]() {
        if (m_qmlProducer && m_filter) {
            seekPreviousSimple();
        }
    });
    connect(this, &KeyframesDock::newFilter, action, [ = ]() {
        action->setEnabled(m_qmlProducer && m_filter);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Seek Next Simple Keyframe"), this);
    action->setObjectName("keyframesSeekNextSimpleAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_BracketRight));
    action->setEnabled(m_qmlProducer && m_filter);
    connect(action, &QAction::triggered, this, [&]() {
        if (m_qmlProducer && m_filter) {
            seekNextSimple();
        }
    });
    connect(this, &KeyframesDock::newFilter, action, [ = ]() {
        action->setEnabled(m_qmlProducer && m_filter);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Toggle Keyframe At Playhead"), this);
    action->setObjectName("keyframesToggleKeyframeAction");
    action->setShortcut(QKeySequence(Qt::Key_Semicolon));
    action->setEnabled(m_qmlProducer && m_filter);
    connect(action, &QAction::triggered, this, [&]() {
        if (m_qmlProducer && m_filter && currentParameter() >= 0) {
            auto position = m_qmlProducer->position() - m_filter->in() - m_qmlProducer->in();
            auto parameterIndex = currentParameter();
            if (m_model.isKeyframe(parameterIndex, position)) {
                auto keyframeIndex = m_model.keyframeIndex(parameterIndex, position);
                m_model.remove(parameterIndex, keyframeIndex);
            } else {
                m_model.addKeyframe(parameterIndex, position);
            }
        }
    });
    connect(this, &KeyframesDock::newFilter, action, [ = ]() {
        action->setEnabled(m_qmlProducer && m_filter);
    });
    m_actions[action->objectName()] = action;
}

int KeyframesDock::seekPrevious()
{
    if (m_qmlProducer) {
        int position = m_model.previousKeyframePosition(currentParameter(),
                                                        m_qmlProducer->position() + m_qmlProducer->in());
        position -= m_qmlProducer->in();
        m_qmlProducer->setPosition(position);
        return m_model.keyframeIndex(currentParameter(),
                                     position + m_qmlProducer->in() - m_filter->in());
    }
    return 0;
}

int KeyframesDock::seekNext()
{
    if (m_qmlProducer) {
        int position = m_model.nextKeyframePosition(currentParameter(),
                                                    m_qmlProducer->position() + m_qmlProducer->in());
        position -= m_qmlProducer->in();
        if (position > m_qmlProducer->position())
            m_qmlProducer->setPosition(position);
        else
            position = m_qmlProducer->position();
        return m_model.keyframeIndex(currentParameter(),
                                     position + m_qmlProducer->in() - m_filter->in());
    }
    return 0;
}

void KeyframesDock::setCurrentFilter(QmlFilter *filter, QmlMetadata *meta)
{
    m_filter = filter;
    m_metadata = meta;
    if (!m_filter || !m_filter->producer().is_valid()) {
        m_filter = &m_emptyQmlFilter;
        m_metadata = &m_emptyQmlMetadata;
    }
    m_model.load(m_filter, m_metadata);
    disconnect(this, SIGNAL(changed()));
    connect(m_filter, SIGNAL(changed()), SIGNAL(changed()));
    connect(m_filter, SIGNAL(changed(QString)), &m_model, SLOT(onFilterChanged(QString)));
    connect(m_filter, SIGNAL(animateInChanged()), &m_model, SLOT(reload()));
    connect(m_filter, SIGNAL(animateOutChanged()), &m_model, SLOT(reload()));
    connect(m_filter, SIGNAL(inChanged(int)), &m_model, SLOT(onFilterInChanged(int)));
    emit newFilter();
}

bool KeyframesDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        load(true);
    }
    return result;
}

void KeyframesDock::keyPressEvent(QKeyEvent *event)
{
    QDockWidget::keyPressEvent(event);
    if (!event->isAccepted())
        MAIN.keyPressEvent(event);
}

void KeyframesDock::keyReleaseEvent(QKeyEvent *event)
{
    QDockWidget::keyReleaseEvent(event);
    if (!event->isAccepted())
        MAIN.keyReleaseEvent(event);
}

void KeyframesDock::onVisibilityChanged(bool visible)
{
    if (visible)
        load();
}

int KeyframesDock::currentParameter() const
{
    if (!m_qview.rootObject())
        return 0;
    return m_qview.rootObject()->property("currentTrack").toInt();
}

void KeyframesDock::load(bool force)
{
    LOG_DEBUG() << "begin";

    if (m_qview.source().isEmpty() || force) {
        QDir viewPath = QmlUtilities::qmlDir();
        viewPath.cd("views");
        viewPath.cd("keyframes");
        m_qview.engine()->addImportPath(viewPath.path());

        QDir modulePath = QmlUtilities::qmlDir();
        modulePath.cd("modules");
        m_qview.engine()->addImportPath(modulePath.path());

        m_qview.setResizeMode(QQuickWidget::SizeRootObjectToView);
        m_qview.quickWindow()->setColor(palette().window().color());
        QUrl source = QUrl::fromLocalFile(viewPath.absoluteFilePath("keyframes.qml"));
        m_qview.setSource(source);
        connect(m_qview.rootObject(), SIGNAL(timeScaleChanged()), this, SIGNAL(timeScaleChanged()));
        connect(m_qview.rootObject(), SIGNAL(rightClicked()),  this, SLOT(onDockRightClicked()));
        connect(m_qview.rootObject(), SIGNAL(keyframeRightClicked()),  this,
                SLOT(onKeyframeRightClicked()));
        connect(m_qview.rootObject(), SIGNAL(clipRightClicked()),  this, SLOT(onClipRightClicked()));
    }
}

void KeyframesDock::onProducerModified()
{
    // The clip name may have changed.
    if (m_qmlProducer)
        emit m_qmlProducer->producerChanged();
}

void KeyframesDock::onDockRightClicked()
{
    m_mainMenu->popup(QCursor::pos());
}

void KeyframesDock::onKeyframeRightClicked()
{
    m_keyMenu->popup(QCursor::pos());
}

void KeyframesDock::onClipRightClicked()
{
    m_clipMenu->popup(QCursor::pos());
}
