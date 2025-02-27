/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (c) 2020-2023 Meltytech, LLC
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#ifndef QMLRICHTEXT_H
#define QMLRICHTEXT_H

#include <QQuickTextDocument>
#include <QtGui/QTextCharFormat>
#include <qqmlfile.h>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

class QmlRichText : public QObject
{
    Q_OBJECT

    Q_ENUMS(HAlignment)

    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(
        int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(
        int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY selectionEndChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
    Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY boldChanged)
    Q_PROPERTY(bool italic READ italic WRITE setItalic NOTIFY italicChanged)
    Q_PROPERTY(bool underline READ underline WRITE setUnderline NOTIFY underlineChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl WRITE setFileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QSizeF size READ size NOTIFY sizeChanged)

public:
    QmlRichText();

    QQuickItem *target() { return m_target; }
    void setTarget(QQuickItem *target);
    void setCursorPosition(int position);
    void setSelectionStart(int position);
    void setSelectionEnd(int position);
    int cursorPosition() const { return m_cursorPosition; }
    int selectionStart() const { return m_selectionStart; }
    int selectionEnd() const { return m_selectionEnd; }
    QString fontFamily() const;
    QColor textColor() const;
    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment a);
    bool bold() const;
    bool italic() const;
    bool underline() const;
    int fontSize() const;
    QUrl fileUrl() const;
    QString text() const;
    QSizeF size() const { return m_doc->size(); }

public slots:
    void setBold(bool arg);
    void setItalic(bool arg);
    void setUnderline(bool arg);
    void setFontSize(int arg);
    void setTextColor(const QColor &arg);
    void setFontFamily(const QString &arg);
    void setFileUrl(const QUrl &arg);
    void setText(const QString &arg);
    void saveAs(const QUrl &arg, QString fileType = QString());
    void insertTable(int rows = 1, int columns = 2, int border = 0);
    void indentLess();
    void indentMore();
    void pastePlain();
    void reset();

signals:
    void targetChanged();
    void cursorPositionChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void fontFamilyChanged();
    void textColorChanged();
    void alignmentChanged();
    void boldChanged();
    void italicChanged();
    void underlineChanged();
    void fontSizeChanged();
    void fileUrlChanged();
    void textChanged();
    void error(QString message);
    void sizeChanged();

private:
    QTextCursor textCursor() const;
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    QQuickItem *m_target;
    QTextDocument *m_doc;
    int m_cursorPosition;
    int m_selectionStart;
    int m_selectionEnd;
    QFont m_font;
    int m_fontSize;
    QUrl m_fileUrl;
    QString m_text;
    QString m_documentTitle;
};

#endif // QMLRICHTEXT_H
