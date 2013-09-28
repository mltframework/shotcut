/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Graphics Dojo project on Qt Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "highlighter.h"

#include <QtWidgets>

Highlighter::Highlighter(QTextDocument *document)
        : QSyntaxHighlighter(document)
{
    m_colors[DocType]        = QColor(192, 192, 192);
    m_colors[Entity]         = QColor(128, 128, 128);
    m_colors[Tag]            = QColor(136,  18, 128);
    m_colors[Comment]        = QColor( 35, 110,  37);
    m_colors[AttributeName]  = QColor(153,  69,   0);
    m_colors[AttributeValue] = QColor( 36,  36, 170);
}

void Highlighter::highlightBlock(const QString &text)
{
    int state = previousBlockState();
    int len = text.length();
    int start = 0;
    int pos = 0;

    while (pos < len) {

        switch (state) {

        case State_Text:
        default:

            while (pos < len) {
                QChar ch = text.at(pos);
                if (ch == '<') {
                    if (text.mid(pos, 4) == "<!--") {
                        state = State_Comment;
                    } else {
                        if (text.mid(pos, 9).toUpper() == "<!DOCTYPE")
                            state = State_DocType;
                        else
                            state = State_TagStart;
                    }
                    break;
                } else if (ch == '&') {
                    start = pos;
                    while (pos < len
                            && text.at(pos++) != ';')
                        ;
                    setFormat(start, pos - start, m_colors[Entity]);
                } else {
                    ++pos;
                }
            }
            break;


        case State_Comment:
            start = pos;
            while (pos < len) {
                if (text.mid(pos, 3) == "-->") {
                    pos += 3;
                    state = State_Text;
                    break;
                } else {
                    ++pos;
                }
            }
            setFormat(start, pos - start, m_colors[Comment]);
            break;

        case State_DocType:
            start = pos;
            while (pos < len) {
                QChar ch = text.at(pos);
                ++pos;
                if (ch == '>') {
                    state = State_Text;
                    break;
                }
            }
            setFormat(start, pos - start, m_colors[DocType]);
            break;

        // at '<' in e.g. "<span>foo</span>"
        case State_TagStart:
            start = pos + 1;
            while (pos < len) {
                QChar ch = text.at(pos);
                ++pos;
                if (ch == '>') {
                    state = State_Text;
                    break;
                }
                if (!ch.isSpace()) {
                    --pos;
                    state = State_TagName;
                    break;
                }
            }
            break;

        // at 'b' in e.g "<blockquote>foo</blockquote>"
        case State_TagName:
            start = pos;
            while (pos < len) {
                QChar ch = text.at(pos);
                ++pos;
                if (ch.isSpace()) {
                    --pos;
                    state = State_InsideTag;
                    break;
                }
                if (ch == '>') {
                    state = State_Text;
                    break;
                }
            }
            setFormat(start, pos - start, m_colors[Tag]);
            break;

        // anywhere after tag name and before tag closing ('>')
        case State_InsideTag:
            start = pos;

            while (pos < len) {
                QChar ch = text.at(pos);
                ++pos;

                if (ch == '/')
                    continue;

                if (ch == '>') {
                    state = State_Text;
                    break;
                }

                if (!ch.isSpace()) {
                    --pos;
                    state = State_AttributeName;
                    break;
                }

            }

            break;

        // at 's' in e.g. <img src=bla.png/>
        case State_AttributeName:
            start = pos;

            while (pos < len) {
                QChar ch = text.at(pos);
                ++pos;

                if (ch == '=') {
                    state = State_AttributeValue;
                    break;
                }

                if (ch == '>' || ch == '/') {
                    state = State_InsideTag;
                    break;
                }
            }

            setFormat(start, pos - start, m_colors[AttributeName]);
            break;

        // after '=' in e.g. <img src=bla.png/>
        case State_AttributeValue:
            start = pos;

            // find first non-space character
            while (pos < len) {
                QChar ch = text.at(pos);
                ++pos;

                // handle opening single quote
                if (ch == '\'') {
                    state = State_SingleQuote;
                    break;
                }

                // handle opening double quote
                if (ch == '"') {
                    state = State_DoubleQuote;
                    break;
                }

                if (!ch.isSpace())
                    break;
            }

            if (state == State_AttributeValue) {
                // attribute value without quote
                // just stop at non-space or tag delimiter
                start = pos;
                while (pos < len) {
                    QChar ch = text.at(pos);
                    if (ch.isSpace())
                        break;
                    if (ch == '>' || ch == '/')
                        break;
                    ++pos;
                }
                state = State_InsideTag;
                setFormat(start, pos - start, m_colors[AttributeValue]);
            }

            break;

        // after the opening single quote in an attribute value
        case State_SingleQuote:
            start = pos;

            while (pos < len) {
                QChar ch = text.at(pos);
                ++pos;
                if (ch == '\'')
                    break;
            }

            state = State_InsideTag;

            setFormat(start, pos - start, m_colors[AttributeValue]);
            break;

        // after the opening double quote in an attribute value
        case State_DoubleQuote:
            start = pos;

            while (pos < len) {
                QChar ch = text.at(pos);
                ++pos;
                if (ch == '"')
                    break;
            }

            state = State_InsideTag;

            setFormat(start, pos - start, m_colors[AttributeValue]);
            break;

        }
    }

    setCurrentBlockState(state);
}
