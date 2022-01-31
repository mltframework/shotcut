/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "notesdock.h"

#include <Logger.h>
#include "mltcontroller.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlview.h"

#include <QAction>
#include <QDir>
#include <QIcon>
#include <QQuickItem>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QUrl>

NotesDock::NotesDock(QWidget *parent) :
    QDockWidget(tr("Notes"), parent),
    m_qview(QmlUtilities::sharedEngine(), this),
    m_blockUpdate(false)
{
    LOG_DEBUG() << "begin";
    setObjectName("NotesDock");
    QIcon filterIcon = QIcon::fromTheme("document-edit", QIcon(":/icons/oxygen/32x32/actions/document-edit.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());
    m_qview.setFocusPolicy(Qt::StrongFocus);
    m_qview.quickWindow()->setPersistentSceneGraph(false);
#ifdef Q_OS_MAC
    setFeatures(DockWidgetClosable | DockWidgetMovable);
#else
    m_qview.setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    QmlUtilities::setCommonProperties(m_qview.rootContext());
    connect(m_qview.quickWindow(), SIGNAL(sceneGraphInitialized()), SLOT(resetQview()));

    QDockWidget::setWidget(&m_qview);

    LOG_DEBUG() << "end";
}

QString NotesDock::getText()
{
    QVariant returnVal;
    QMetaObject::invokeMethod(m_qview.rootObject(), "getText", Qt::DirectConnection, Q_RETURN_ARG(QVariant, returnVal));
    return returnVal.toString();
}

void NotesDock::setText(const QString& text)
{
    m_blockUpdate = true;
    QMetaObject::invokeMethod(m_qview.rootObject(), "setText", Q_ARG(QVariant, QVariant(text)));
    m_blockUpdate = false;
}

bool NotesDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        resetQview();
    }
    return result;
}

void NotesDock::keyPressEvent(QKeyEvent *event)
{
    QDockWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_F) {
        event->ignore();
    } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        event->accept();
    }
}

void NotesDock::resetQview()
{
    LOG_DEBUG() << "begin";
    if (m_qview.status() != QQuickWidget::Null) {
        QObject* root = m_qview.rootObject();
        QObject::disconnect(root, SIGNAL(textChanged(const QString&)), this, SLOT(onTextChanged(const QString&)));
        QObject::disconnect(root, SIGNAL(minWidthChanged()), this, SLOT(onMinimumWidthChanged()));
        m_qview.setSource(QUrl(""));
    }

    QDir modulePath = QmlUtilities::qmlDir();
    modulePath.cd("modules");
    m_qview.engine()->addImportPath(modulePath.path());

    m_qview.setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_qview.quickWindow()->setColor(palette().window().color());
    QDir controlPath = QmlUtilities::qmlDir();
    controlPath.cd("modules");
    controlPath.cd("Shotcut");
    controlPath.cd("Controls");
    QUrl source = QUrl::fromLocalFile(controlPath.absoluteFilePath("RichTextEditor.qml"));
    m_qview.setSource(source);

    m_qview.rootObject()->setProperty("allowRichText", false);
    QObject::connect(m_qview.rootObject(), SIGNAL(textModified(QString)), SLOT(onTextChanged(QString)));
    QObject::connect(m_qview.rootObject(), SIGNAL(minWidthChanged()), SLOT(onMinimumWidthChanged()));
}

void NotesDock::onTextChanged(QString /*text*/)
{
    if (!m_blockUpdate) {
        emit modified();
    }
}

void NotesDock::onMinimumWidthChanged() {
    m_qview.setMinimumWidth(m_qview.rootObject()->property("minWidth").toInt());
}
