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

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QHash>
#include <QSyntaxHighlighter>

// based on http://doc.trolltech.com/qq/qq21-syntaxhighlighter.html

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:

    Highlighter(QTextDocument *document);

    enum Construct {
        DocType,
        Entity,
        Tag,
        Comment,
        AttributeName,
        AttributeValue
    };

protected:
    enum State {
        State_Text = -1,
        State_DocType,
        State_Comment,
        State_TagStart,
        State_TagName,
        State_InsideTag,
        State_AttributeName,
        State_SingleQuote,
        State_DoubleQuote,
        State_AttributeValue,
    };

    void highlightBlock(const QString &text);

private:
    QHash<int, QColor> m_colors;
};


#endif // HIGHLIGHTER_H
