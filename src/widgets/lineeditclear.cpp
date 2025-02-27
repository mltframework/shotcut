/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
** Copyright (c) 2024 Meltytech, LLC
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/

#include "lineeditclear.h"

#include <QKeyEvent>
#include <QStyle>
#include <QToolButton>

LineEditClear::LineEditClear(QWidget *parent)
    : QLineEdit(parent)
{
    clearButton = new QToolButton(this);
    clearButton->setIcon(
        QIcon::fromTheme("edit-clear", QIcon(":/icons/oxygen/32x32/actions/edit-clear.png")));
    //    clearButton->setIconSize(QSize(16, 16));
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    clearButton->hide();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(this,
            SIGNAL(textChanged(const QString &)),
            this,
            SLOT(updateCloseButton(const QString &)));
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    //    setStyleSheet(QStringLiteral("QLineEdit { padding-right: %1px; } ").arg(clearButton->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
    installEventFilter(this);
}

void LineEditClear::resizeEvent(QResizeEvent *)
{
    QSize sz = clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    clearButton->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height()) / 2);
}

bool LineEditClear::eventFilter(QObject *target, QEvent *event)
{
    if (QEvent::KeyPress == event->type()
        && Qt::Key_Escape == static_cast<QKeyEvent *>(event)->key()) {
        clear();
    }
    return QLineEdit::eventFilter(target, event);
}

void LineEditClear::updateCloseButton(const QString &text)
{
    clearButton->setVisible(!text.isEmpty());
}
