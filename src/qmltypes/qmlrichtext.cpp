/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (c) 2020-2024 Meltytech, LLC
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

#include "qmlrichtext.h"

#include "Logger.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QStringBuilder>
#include <QStringConverter>
#include <QtCore/QFileInfo>
#include <QtGui/QFontDatabase>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>

QmlRichText::QmlRichText()
    : m_target(0)
    , m_doc(0)
    , m_cursorPosition(-1)
    , m_selectionStart(0)
    , m_selectionEnd(0)
{}

void QmlRichText::setTarget(QQuickItem *target)
{
    m_doc = 0;
    m_target = target;
    if (!m_target)
        return;

    QVariant doc = m_target->property("textDocument");
    if (doc.canConvert<QQuickTextDocument *>()) {
        QQuickTextDocument *qqdoc = doc.value<QQuickTextDocument *>();
        if (qqdoc) {
            m_doc = qqdoc->textDocument();
            connect(m_doc, &QTextDocument::contentsChanged, this, &QmlRichText::sizeChanged);
        }
    }
    emit targetChanged();
}

void QmlRichText::setFileUrl(const QUrl &arg)
{
    if (m_fileUrl != arg) {
        m_fileUrl = arg;
        QString fileName = QQmlFile::urlToLocalFileOrQrc(arg);
        if (QFile::exists(fileName)) {
            QFile file(fileName);
            if (file.open(QFile::ReadOnly)) {
                QByteArray data = file.readAll();
                if (Qt::mightBeRichText(data)) {
                    auto decoder = QStringDecoder(
                        QStringConverter::encodingForHtml(data).value_or(QStringConverter::Utf8));
                    setText(decoder(data));
                } else {
                    auto decoder = QStringDecoder(
                        QStringConverter::encodingForData(data).value_or(QStringConverter::Utf8));
                    setText(QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
                                           "\"http://www.w3.org/TR/REC-html40/strict.dtd\">"
                                           "<html><head><meta name=\"qrichtext\" content=\"1\" "
                                           "/><style type=\"text/css\">"
                                           "p, li { white-space: pre-wrap; }"
#if defined(Q_OS_WIN)
                                           "body { font-family:Verdana; font-size:72pt; "
                                           "font-weight:normal; font-style:normal; color:#ffffff; }"
#elif defined(Q_OS_MAC)
                                           "body { font-family:Helvetica; font-size:72pt; "
                                           "font-weight:normal; font-style:normal; color:#ffffff; }"
#else
                                           "body { font-family:sans-serif; font-size:72pt; "
                                           "font-weight:normal; font-style:normal; color:#ffffff; }"
#endif
                                           "</style></head><body>")
                            % QString(decoder(data)) % QStringLiteral("</body></html>"));
                }
                if (m_doc)
                    m_doc->setModified(false);
                if (fileName.isEmpty())
                    m_documentTitle = QStringLiteral("untitled.txt");
                else
                    m_documentTitle = QFileInfo(fileName).fileName();

                emit textChanged();
                reset();
            }
        }
        emit fileUrlChanged();
    }
}

void QmlRichText::setText(const QString &arg)
{
    if (m_text != arg) {
        m_text = arg;
        emit textChanged();
    }
}

void QmlRichText::saveAs(const QUrl &arg, QString fileType)
{
    if (fileType.isEmpty())
        fileType = QFileInfo(arg.toString()).suffix();
    bool isHtml = fileType.contains(QLatin1String("htm"));
    QLatin1String ext(isHtml ? ".html" : ".txt");
    QString localPath = arg.toLocalFile();
    if (!localPath.endsWith(ext))
        localPath += ext;
    QFile f(localPath);
    if (!f.open(QFile::WriteOnly | QFile::Truncate | (isHtml ? QFile::NotOpen : QFile::Text))) {
        emit error(tr("Cannot save: ") + f.errorString());
        return;
    }
    QString s = isHtml ? m_doc->toHtml() : m_doc->toPlainText();
    f.write(s.toUtf8());
    f.close();
    setFileUrl(QUrl::fromLocalFile(localPath));
}

void QmlRichText::insertTable(int rows, int columns, int border)
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return;
    QString color = textColor().name(QColor::HexArgb);
    QString html = QString("<style>"
                           "table { border-style: solid; border-color: %1 }"
                           "td { font: %2 %3 %4pt %5;"
                           "color: %1; vertical-align: top; }"
                           "</style>"
                           "<table width=100% cellspacing=0 cellpadding=%6 border=%6>")
                       .arg(color)
                       .arg(italic() ? "italic" : "normal")
                       .arg(bold() ? "bold" : "normal")
                       .arg(fontSize())
                       .arg(fontFamily())
                       .arg(border);
    for (auto i = 0; i < rows; ++i) {
        html += "<tr>";
        for (auto j = 0; j < columns; ++j) {
            if (j == 0) {
                html += QStringLiteral("<td>%1 %2</td>").arg(tr("Row")).arg(i + 1);
            } else {
                html += QStringLiteral("<td>%1 %2</td>").arg(tr("Column")).arg(j + 1);
            }
            if (border == 0 && j + 1 < columns) {
                html += "<td width=5%></td>";
            }
        }
        html += "</tr>";
    }
    html += "</table>";
    cursor.insertHtml(html);
}

void QmlRichText::indentLess()
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return;
    auto indent = cursor.blockFormat().indent();
    QTextBlockFormat format;
    format.setIndent(qMax(indent - 1, 0));
    cursor.mergeBlockFormat(format);
}

void QmlRichText::indentMore()
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return;
    auto indent = cursor.blockFormat().indent();
    QTextBlockFormat format;
    format.setIndent(indent + 1);
    cursor.mergeBlockFormat(format);
}

void QmlRichText::pastePlain()
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return;
    cursor.insertText(QGuiApplication::clipboard()->text());
}

QUrl QmlRichText::fileUrl() const
{
    return m_fileUrl;
}

QString QmlRichText::text() const
{
    return m_text;
}

void QmlRichText::setCursorPosition(int position)
{
    if (position == m_cursorPosition)
        return;

    m_cursorPosition = position;

    reset();
}

void QmlRichText::reset()
{
    emit fontFamilyChanged();
    emit alignmentChanged();
    emit boldChanged();
    emit italicChanged();
    emit underlineChanged();
    emit fontSizeChanged();
    emit textColorChanged();
}

QTextCursor QmlRichText::textCursor() const
{
    if (!m_doc)
        return QTextCursor();

    QTextCursor cursor = QTextCursor(m_doc);
    if (m_selectionStart != m_selectionEnd) {
        cursor.setPosition(m_selectionStart);
        cursor.setPosition(m_selectionEnd, QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(m_cursorPosition);
    }
    return cursor;
}

void QmlRichText::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
}

void QmlRichText::setSelectionStart(int position)
{
    m_selectionStart = position;
}

void QmlRichText::setSelectionEnd(int position)
{
    m_selectionEnd = position;
}

void QmlRichText::setAlignment(Qt::Alignment a)
{
    if (!m_doc)
        return;

    QTextBlockFormat fmt;
    fmt.setAlignment((Qt::Alignment) a);
    QTextCursor cursor = QTextCursor(m_doc);
    cursor.setPosition(m_selectionStart, QTextCursor::MoveAnchor);
    cursor.setPosition(m_selectionEnd, QTextCursor::KeepAnchor);
    cursor.mergeBlockFormat(fmt);
    emit alignmentChanged();
}

Qt::Alignment QmlRichText::alignment() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return Qt::AlignLeft;
    return textCursor().blockFormat().alignment();
}

bool QmlRichText::bold() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return false;
    return textCursor().charFormat().fontWeight() == QFont::Bold;
}

bool QmlRichText::italic() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return false;
    return textCursor().charFormat().fontItalic();
}

bool QmlRichText::underline() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return false;
    return textCursor().charFormat().fontUnderline();
}

void QmlRichText::setBold(bool arg)
{
    QTextCharFormat fmt;
    fmt.setFontWeight(arg ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
    emit boldChanged();
}

void QmlRichText::setItalic(bool arg)
{
    QTextCharFormat fmt;
    fmt.setFontItalic(arg);
    mergeFormatOnWordOrSelection(fmt);
    emit italicChanged();
}

void QmlRichText::setUnderline(bool arg)
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(arg);
    mergeFormatOnWordOrSelection(fmt);
    emit underlineChanged();
}

int QmlRichText::fontSize() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return 0;
    QTextCharFormat format = cursor.charFormat();
    return format.font().pointSize();
}

void QmlRichText::setFontSize(int arg)
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return;
    QTextCharFormat format;
    format.setFontPointSize(arg);
    mergeFormatOnWordOrSelection(format);
    emit fontSizeChanged();
}

QColor QmlRichText::textColor() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return QColor(Qt::black);
    QTextCharFormat format = cursor.charFormat();
    return format.foreground().color();
}

void QmlRichText::setTextColor(const QColor &c)
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return;
    QTextCharFormat format;
    format.setForeground(QBrush(c));
    mergeFormatOnWordOrSelection(format);
    emit textColorChanged();
}

QString QmlRichText::fontFamily() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return QString();
    QTextCharFormat format = cursor.charFormat();
    return format.font().family();
}

void QmlRichText::setFontFamily(const QString &arg)
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return;
    QTextCharFormat format;
    format.setFontFamilies({arg});
    mergeFormatOnWordOrSelection(format);
    emit fontFamilyChanged();
}
