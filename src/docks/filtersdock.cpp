/*
 * Copyright (c) 2013-2024 Meltytech, LLC
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

#include "filtersdock.h"
#include <QQuickView>
#include <QQuickItem>
#include <QtWidgets/QScrollArea>
#include <QQmlEngine>
#include <QDir>
#include <QMenu>
#include <QUrl>
#include <QQmlContext>
#include <QAction>
#include <QIcon>
#include <Logger.h>

#include "actions.h"
#include "mainwindow.h"
#include "controllers/filtercontroller.h"
#include "qmltypes/qmlapplication.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlview.h"
#include "models/metadatamodel.h"
#include "models/motiontrackermodel.h"
#include "models/subtitlesmodel.h"
#include "models/attachedfiltersmodel.h"
#include "mltcontroller.h"

FiltersDock::FiltersDock(MetadataModel *metadataModel, AttachedFiltersModel *attachedModel,
                         MotionTrackerModel *motionTrackerModel, SubtitlesModel *subtitlesModel,
                         QWidget *parent) :
    QDockWidget(tr("Filters"), parent),
    m_qview(QmlUtilities::sharedEngine(), this)
{
    LOG_DEBUG() << "begin";
    setObjectName("FiltersDock");
    QIcon filterIcon = QIcon::fromTheme("view-filter",
                                        QIcon(":/icons/oxygen/32x32/actions/view-filter.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());
    setMinimumSize(200, 200);
    setupActions();

    m_qview.setFocusPolicy(Qt::StrongFocus);
    m_qview.quickWindow()->setPersistentSceneGraph(false);
#ifndef Q_OS_MAC
    m_qview.setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    setWidget(&m_qview);

    QmlUtilities::setCommonProperties(m_qview.rootContext());
    m_qview.rootContext()->setContextProperty("view", new QmlView(&m_qview));
    m_qview.rootContext()->setContextProperty("metadatamodel", metadataModel);
    m_qview.rootContext()->setContextProperty("motionTrackerModel", motionTrackerModel);
    m_qview.rootContext()->setContextProperty("subtitlesModel", subtitlesModel);
    m_qview.rootContext()->setContextProperty("attachedfiltersmodel", attachedModel);
    m_qview.rootContext()->setContextProperty("producer", &m_producer);
    connect(&m_producer, SIGNAL(seeked(int)), SIGNAL(seeked(int)));
    connect(this, SIGNAL(producerInChanged(int)), &m_producer, SIGNAL(inChanged(int)));
    connect(this, SIGNAL(producerOutChanged(int)), &m_producer, SIGNAL(outChanged(int)));
    connect(m_qview.quickWindow(), &QQuickWindow::sceneGraphInitialized, this, &FiltersDock::load,
            Qt::QueuedConnection);

    setCurrentFilter(0, 0, QmlFilter::NoCurrentFilter);

    LOG_DEBUG() << "end";
}

void FiltersDock::setCurrentFilter(QmlFilter *filter, QmlMetadata *meta, int index)
{
    if (filter && filter->producer().is_valid()) {
        m_producer.setProducer(filter->producer());
        if (mlt_service_playlist_type != filter->producer().type() && MLT.producer()
                && MLT.producer()->is_valid())
            m_producer.seek(MLT.producer()->position());
    } else {
        Mlt::Producer emptyProducer(mlt_producer(0));
        m_producer.setProducer(emptyProducer);
    }
    m_qview.rootContext()->setContextProperty("filter", filter);
    m_qview.rootContext()->setContextProperty("metadata", meta);
    if (filter)
        connect(filter, SIGNAL(changed(QString)), SIGNAL(changed()));
    else
        disconnect(this, SIGNAL(changed()));
    QMetaObject::invokeMethod(m_qview.rootObject(), "setCurrentFilter", Q_ARG(QVariant,
                                                                              QVariant(index)));
}

bool FiltersDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange
            || event->type() == QEvent::Show) {
        load();
    }
    return result;
}

void FiltersDock::keyPressEvent(QKeyEvent *event)
{
    QDockWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_F) {
        event->ignore();
    } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        event->accept();
    }
}

void FiltersDock::onSeeked(int position)
{
    if (m_producer.producer().is_valid()) {
        if (MLT.isMultitrack()) {
            // Make the position relative to clip's position on a timeline track.
            position -= m_producer.producer().get_int(kPlaylistStartProperty);
        } else {
            // Make the position relative to the clip's in point.
            position -= m_producer.in();
        }
        m_producer.seek(position);
    }
}

void FiltersDock::onShowFrame(const SharedFrame &frame)
{
    if (m_producer.producer().is_valid()) {
        int position = frame.get_position();
        onSeeked(position);
    }
}

void FiltersDock::openFilterMenu() const
{
    QMetaObject::invokeMethod(m_qview.rootObject(), "openFilterMenu");
}

void FiltersDock::showCopyFilterMenu()
{
    QMenu menu;
    menu.addAction(Actions["filtersCopyCurrentFilterAction"]);
    menu.addAction(Actions["filtersCopyFiltersAction"]);
    menu.addAction(Actions["filtersCopyAllFilterAction"]);
    menu.exec(QCursor::pos());
}

void FiltersDock::onServiceInChanged(int delta, Mlt::Service *service)
{
    if (delta && service && m_producer.producer().is_valid()
            && service->get_service() == m_producer.producer().get_service()) {
        emit producerInChanged(delta);
    }
}

void FiltersDock::load()
{
    if (!m_qview.quickWindow()->isSceneGraphInitialized() && loadTries++ < 5) {
        LOG_WARNING() << "scene graph not yet initialized";
        QTimer::singleShot(300, this, &FiltersDock::load);
    }

    LOG_DEBUG() << "begin" << "isVisible" << isVisible() << "qview.status" << m_qview.status();

    emit currentFilterRequested(QmlFilter::NoCurrentFilter);

    QDir viewPath = QmlUtilities::qmlDir();
    viewPath.cd("views");
    viewPath.cd("filter");
    m_qview.engine()->addImportPath(viewPath.path());

    QDir modulePath = QmlUtilities::qmlDir();
    modulePath.cd("modules");
    m_qview.engine()->addImportPath(modulePath.path());

    m_qview.setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_qview.quickWindow()->setColor(palette().window().color());
    QUrl source = QUrl::fromLocalFile(viewPath.absoluteFilePath("filterview.qml"));
    m_qview.setSource(source);

    QObject::connect(m_qview.rootObject(), SIGNAL(currentFilterRequested(int)),
                     SIGNAL(currentFilterRequested(int)));
    QObject::connect(m_qview.rootObject(), SIGNAL(copyFilterRequested()),
                     SLOT(showCopyFilterMenu()));
}

void FiltersDock::setupActions()
{
    QIcon icon;
    QAction *action;

    action = new QAction(tr("Add"), this);
    action->setShortcut(QKeySequence(Qt::Key_F));
    action->setToolTip(tr("Choose a filter to add"));
    icon = QIcon::fromTheme("list-add",
                            QIcon(":/icons/oxygen/32x32/actions/list-add.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [ = ]() {
        show();
        raise();
        m_qview.setFocus();
        openFilterMenu();
    });
    addAction(action);
    Actions.add("filtersAddFilterAction", action, windowTitle());

    action = new QAction(tr("Remove"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F));
    action->setToolTip(tr("Remove selected filter"));
    icon = QIcon::fromTheme("list-remove",
                            QIcon(":/icons/oxygen/32x32/actions/list-remove.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [ = ]() {
        MAIN.filterController()->removeCurrent();
    });
    addAction(action);
    Actions.add("filtersRemoveFilterAction", action, windowTitle());

    action = new QAction(tr("Copy Enabled"), this);
    action->setToolTip(tr("Copy checked filters to the clipboard"));
    connect(action, &QAction::triggered, this, [ = ]() {
        QmlApplication::singleton().copyEnabledFilters();
    });
    addAction(action);
    Actions.add("filtersCopyFiltersAction", action, windowTitle());

    action = new QAction(tr("Copy Current"), this);
    action->setToolTip(tr("Copy current filter to the clipboard"));
    connect(action, &QAction::triggered, this, [ = ]() {
        QmlApplication::singleton().copyCurrentFilter();
    });
    addAction(action);
    Actions.add("filtersCopyCurrentFilterAction", action, windowTitle());

    action = new QAction(tr("Copy All"), this);
    action->setToolTip(tr("Copy all filters to the clipboard"));
    connect(action, &QAction::triggered, this, [ = ]() {
        QmlApplication::singleton().copyAllFilters();
    });
    addAction(action);
    Actions.add("filtersCopyAllFilterAction", action, windowTitle());

    action = new QAction(tr("Paste Filters"), this);
    action->setToolTip(tr("Paste the filters from the clipboard"));
    icon = QIcon::fromTheme("edit-paste",
                            QIcon(":/icons/oxygen/32x32/actions/edit-paste.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [ = ]() {
        QmlApplication::singleton().pasteFilters();
    });
    addAction(action);
    Actions.add("filtersPasteFiltersAction", action, windowTitle());
}
