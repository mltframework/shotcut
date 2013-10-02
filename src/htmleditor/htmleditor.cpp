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


#include "htmleditor.h"
#include "highlighter.h"

#include "ui_htmleditor.h"
#include "ui_inserthtmldialog.h"

#include <QtWidgets>
#include <QtWebKitWidgets>
#include <QQuickView>

#define FORWARD_ACTION(action1, action2) \
    connect(action1, SIGNAL(triggered()), \
            ui->webView->pageAction(action2), SLOT(trigger())); \
    connect(ui->webView->pageAction(action2), \
            SIGNAL(changed()), SLOT(adjustActions()));


HtmlEditor::HtmlEditor(QWidget *parent)
        : QWidget(parent)
        , ui(new Ui_HtmlEditor)
        , sourceDirty(true)
        , highlighter(0)
        , ui_dialog(0)
        , insertHtmlDialog(0)
{
    ui->setupUi(this);

    QPalette pal = ui->webView->page()->palette();
    pal.setColor(QPalette::Base, Qt::gray);
    ui->webView->page()->setPalette(pal);

    ui->tabWidget->setTabText(0, tr("WYSIWYG Editor"));
    ui->tabWidget->setTabText(1, tr("View Source"));
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), SLOT(changeTab(int)));

    highlighter = new Highlighter(ui->plainTextEdit->document());

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->standardToolBar->insertWidget(ui->actionZoomOut, spacer);

    zoomLabel = new QLabel;
    ui->standardToolBar->insertWidget(ui->actionZoomOut, zoomLabel);

    zoomSlider = new QSlider(this);
    zoomSlider->setOrientation(Qt::Horizontal);
    zoomSlider->setMaximumWidth(150);
    zoomSlider->setRange(25, 400);
    zoomSlider->setSingleStep(25);
    zoomSlider->setPageStep(100);
    connect(zoomSlider, SIGNAL(valueChanged(int)), SLOT(changeZoom(int)));
    ui->standardToolBar->insertWidget(ui->actionZoomIn, zoomSlider);

    connect(ui->actionFileNew, SIGNAL(triggered()), SLOT(fileNew()));
    connect(ui->actionFileOpen, SIGNAL(triggered()), SLOT(fileOpen()));
    connect(ui->actionFileSave, SIGNAL(triggered()), SLOT(fileSave()));
    connect(ui->actionFileSaveAs, SIGNAL(triggered()), SLOT(fileSaveAs()));
    connect(ui->actionExit, SIGNAL(triggered()), SLOT(close()));
    connect(ui->actionInsertImage, SIGNAL(triggered()), SLOT(insertImage()));
    connect(ui->actionCreateLink, SIGNAL(triggered()), SLOT(createLink()));
    connect(ui->actionInsertHtml, SIGNAL(triggered()), SLOT(insertHtml()));
    connect(ui->actionZoomOut, SIGNAL(triggered()), SLOT(zoomOut()));
    connect(ui->actionZoomIn, SIGNAL(triggered()), SLOT(zoomIn()));

    // these are forward to internal QWebView
    FORWARD_ACTION(ui->actionEditUndo, QWebPage::Undo);
    FORWARD_ACTION(ui->actionEditRedo, QWebPage::Redo);
    FORWARD_ACTION(ui->actionEditCut, QWebPage::Cut);
    FORWARD_ACTION(ui->actionEditCopy, QWebPage::Copy);
    FORWARD_ACTION(ui->actionEditPaste, QWebPage::Paste);
    FORWARD_ACTION(ui->actionFormatBold, QWebPage::ToggleBold);
    FORWARD_ACTION(ui->actionFormatItalic, QWebPage::ToggleItalic);
    FORWARD_ACTION(ui->actionFormatUnderline, QWebPage::ToggleUnderline);

    // Qt 4.5.0 has a bug: always returns 0 for QWebPage::SelectAll
    connect(ui->actionEditSelectAll, SIGNAL(triggered()), SLOT(editSelectAll()));

    connect(ui->actionStyleParagraph, SIGNAL(triggered()), SLOT(styleParagraph()));
    connect(ui->actionStyleHeading1, SIGNAL(triggered()), SLOT(styleHeading1()));
    connect(ui->actionStyleHeading2, SIGNAL(triggered()), SLOT(styleHeading2()));
    connect(ui->actionStyleHeading3, SIGNAL(triggered()), SLOT(styleHeading3()));
    connect(ui->actionStyleHeading4, SIGNAL(triggered()), SLOT(styleHeading4()));
    connect(ui->actionStyleHeading5, SIGNAL(triggered()), SLOT(styleHeading5()));
    connect(ui->actionStyleHeading6, SIGNAL(triggered()), SLOT(styleHeading6()));
    connect(ui->actionStylePreformatted, SIGNAL(triggered()), SLOT(stylePreformatted()));
    connect(ui->actionStyleAddress, SIGNAL(triggered()), SLOT(styleAddress()));
    connect(ui->actionFormatFontName, SIGNAL(triggered()), SLOT(formatFontName()));
    connect(ui->actionFormatFontSize, SIGNAL(triggered()), SLOT(formatFontSize()));
    connect(ui->actionFormatTextColor, SIGNAL(triggered()), SLOT(formatTextColor()));
    connect(ui->actionFormatBackgroundColor, SIGNAL(triggered()), SLOT(formatBackgroundColor()));

    // no page action exists yet for these, so use execCommand trick
    connect(ui->actionFormatStrikethrough, SIGNAL(triggered()), SLOT(formatStrikeThrough()));
    connect(ui->actionFormatAlignLeft, SIGNAL(triggered()), SLOT(formatAlignLeft()));
    connect(ui->actionFormatAlignCenter, SIGNAL(triggered()), SLOT(formatAlignCenter()));
    connect(ui->actionFormatAlignRight, SIGNAL(triggered()), SLOT(formatAlignRight()));
    connect(ui->actionFormatAlignJustify, SIGNAL(triggered()), SLOT(formatAlignJustify()));
    connect(ui->actionFormatDecreaseIndent, SIGNAL(triggered()), SLOT(formatDecreaseIndent()));
    connect(ui->actionFormatIncreaseIndent, SIGNAL(triggered()), SLOT(formatIncreaseIndent()));
    connect(ui->actionFormatNumberedList, SIGNAL(triggered()), SLOT(formatNumberedList()));
    connect(ui->actionFormatBulletedList, SIGNAL(triggered()), SLOT(formatBulletedList()));

    // necessary to sync our actions
    connect(ui->webView->page(), SIGNAL(selectionChanged()), SLOT(adjustActions()));
    connect(ui->webView->page(), SIGNAL(contentsChanged()), SLOT(adjustSource()));
    ui->webView->setFocus();

    setCurrentFileName(QString());
    fileNew();

    adjustActions();
    adjustSource();
    setWindowModified(false);
//    changeZoom(100);
}

HtmlEditor::~HtmlEditor()
{
    delete ui;
    delete ui_dialog;
}

bool HtmlEditor::maybeSave()
{
    if (!isWindowModified())
        return true;

    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("HTML Editor"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard
                               | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

void HtmlEditor::fileNew()
{
    if (maybeSave()) {
        ui->webView->setHtml("<p></p>");
        ui->webView->setFocus();
        ui->webView->page()->setContentEditable(true);
        setCurrentFileName(QString());
        setWindowModified(false);

        // quirk in QWebView: need an initial mouse click to show the cursor
        int mx = ui->webView->width() / 2;
        int my = ui->webView->height() / 2;
        QPoint center = QPoint(mx, my);
        QMouseEvent *e1 = new QMouseEvent(QEvent::MouseButtonPress, center,
                                          Qt::LeftButton, Qt::LeftButton,
                                          Qt::NoModifier);
        QMouseEvent *e2 = new QMouseEvent(QEvent::MouseButtonRelease, center,
                                          Qt::LeftButton, Qt::LeftButton,
                                          Qt::NoModifier);
        QApplication::postEvent(ui->webView, e1);
        QApplication::postEvent(ui->webView, e2);
    }
}

void HtmlEditor::fileOpen()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open File..."),
                 QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (!fn.isEmpty())
        load(fn);
}

bool HtmlEditor::fileSave()
{
    if (fileName.isEmpty() || fileName.startsWith(QLatin1String(":/")))
        return fileSaveAs();

    QFile file(fileName);
    bool success = file.open(QIODevice::WriteOnly);
    if (success) {
        // FIXME: here we always use UTF-8 encoding
        QString content = ui->webView->page()->mainFrame()->toHtml();
        QByteArray data = content.toUtf8();
        qint64 c = file.write(data);
        success = (c >= data.length());
    }

    setWindowModified(false);
    emit saved();
    return success;
}

bool HtmlEditor::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                 QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    if (!(fn.endsWith(".htm", Qt::CaseInsensitive) || fn.endsWith(".html", Qt::CaseInsensitive)))
        fn += ".htm"; // default
    setCurrentFileName(fn);
    return fileSave();
}

void HtmlEditor::insertImage()
{
    QString filters;
    filters += tr("Common Graphics (*.png *.jpg *.jpeg *.gif);;");
    filters += tr("Portable Network Graphics (PNG) (*.png);;");
    filters += tr("JPEG (*.jpg *.jpeg);;");
    filters += tr("Graphics Interchange Format (*.gif);;");
    filters += tr("All Files (*)");

    QString fn = QFileDialog::getOpenFileName(this, tr("Open image..."),
                 QString(), filters);
    if (fn.isEmpty())
        return;
    if (!QFile::exists(fn))
        return;

    QUrl url = QUrl::fromLocalFile(fn);
    execCommand("insertImage", url.toString());
}

// shamelessly copied from Qt Demo Browser
static QUrl guessUrlFromString(const QString &string)
{
    QString urlStr = string.trimmed();
    QRegExp test(QLatin1String("^[a-zA-Z]+\\:.*"));

    // Check if it looks like a qualified URL. Try parsing it and see.
    bool hasSchema = test.exactMatch(urlStr);
    if (hasSchema) {
        QUrl url(urlStr, QUrl::TolerantMode);
        if (url.isValid())
            return url;
    }

    // Might be a file.
    if (QFile::exists(urlStr))
        return QUrl::fromLocalFile(urlStr);

    // Might be a shorturl - try to detect the schema.
    if (!hasSchema) {
        int dotIndex = urlStr.indexOf(QLatin1Char('.'));
        if (dotIndex != -1) {
            QString prefix = urlStr.left(dotIndex).toLower();
            QString schema = (prefix == QLatin1String("ftp")) ? prefix : QLatin1String("http");
            QUrl url(schema + QLatin1String("://") + urlStr, QUrl::TolerantMode);
            if (url.isValid())
                return url;
        }
    }

    // Fall back to QUrl's own tolerant parser.
    return QUrl(string, QUrl::TolerantMode);
}

void HtmlEditor::createLink()
{
    QString link = QInputDialog::getText(this, tr("Create link"),
                                         tr("Enter URL"));
    if (!link.isEmpty()) {
        QUrl url = guessUrlFromString(link);
        if (url.isValid())
            execCommand("createLink", url.toString());
    }
}

void HtmlEditor::insertHtml()
{
    if (!insertHtmlDialog) {
        insertHtmlDialog = new QDialog(this);
        if (!ui_dialog)
            ui_dialog = new Ui_Dialog;
        ui_dialog->setupUi(insertHtmlDialog);
        connect(ui_dialog->buttonBox, SIGNAL(accepted()),
                insertHtmlDialog, SLOT(accept()));
        connect(ui_dialog->buttonBox, SIGNAL(rejected()),
                insertHtmlDialog, SLOT(reject()));
    }

    ui_dialog->plainTextEdit->clear();
    ui_dialog->plainTextEdit->setFocus();
    Highlighter *hilite = new Highlighter(ui_dialog->plainTextEdit->document());

    if (insertHtmlDialog->exec() == QDialog::Accepted)
        execCommand("insertHTML", ui_dialog->plainTextEdit->toPlainText());

    delete hilite;
}

void HtmlEditor::zoomOut()
{
    int percent = static_cast<int>(ui->webView->zoomFactor() * 100);
    if (percent > 25) {
        percent -= 25;
        percent = 25 * (int((percent + 25 - 1) / 25));
        qreal factor = static_cast<qreal>(percent) / 100;
        ui->webView->setZoomFactor(factor);
        ui->actionZoomOut->setEnabled(percent > 25);
        ui->actionZoomIn->setEnabled(true);
        zoomSlider->setValue(percent);
    }
}

void HtmlEditor::zoomIn()
{
    int percent = static_cast<int>(ui->webView->zoomFactor() * 100);
    if (percent < 400) {
        percent += 25;
        percent = 25 * (int(percent / 25));
        qreal factor = static_cast<qreal>(percent) / 100;
        ui->webView->setZoomFactor(factor);
        ui->actionZoomIn->setEnabled(percent < 400);
        ui->actionZoomOut->setEnabled(true);
        zoomSlider->setValue(percent);
    }
}

void HtmlEditor::editSelectAll()
{
    ui->webView->triggerPageAction(QWebPage::SelectAll);
}

void HtmlEditor::execCommand(const QString &cmd)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("document.execCommand(\"%1\", false, null)").arg(cmd);
    frame->evaluateJavaScript(js);
}

void HtmlEditor::execCommand(const QString &cmd, const QString &arg)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("document.execCommand(\"%1\", false, \"%2\")").arg(cmd).arg(arg);
    frame->evaluateJavaScript(js);
}

bool HtmlEditor::queryCommandState(const QString &cmd)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("document.queryCommandState(\"%1\", false, null)").arg(cmd);
    QVariant result = frame->evaluateJavaScript(js);
    return result.toString().simplified().toLower() == "true";
}

QString HtmlEditor::qmlFilePath(const QString &fileName)
{
    QDir dir(qApp->applicationDirPath());
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    dir.cdUp();
#endif
    dir.cd("share");
    dir.cd("shotcut");
    dir.cd("qml");
    dir.cd("htmleditor");
    return dir.absoluteFilePath(fileName);
}

void HtmlEditor::styleParagraph()
{
    execCommand("formatBlock", "p");
}

void HtmlEditor::styleHeading1()
{
    execCommand("formatBlock", "h1");
}

void HtmlEditor::styleHeading2()
{
    execCommand("formatBlock", "h2");
}

void HtmlEditor::styleHeading3()
{
    execCommand("formatBlock", "h3");
}

void HtmlEditor::styleHeading4()
{
    execCommand("formatBlock", "h4");
}

void HtmlEditor::styleHeading5()
{
    execCommand("formatBlock", "h5");
}

void HtmlEditor::styleHeading6()
{
    execCommand("formatBlock", "h6");
}

void HtmlEditor::stylePreformatted()
{
    execCommand("formatBlock", "pre");
}

void HtmlEditor::styleAddress()
{
    execCommand("formatBlock", "address");
}

void HtmlEditor::formatStrikeThrough()
{
    execCommand("strikeThrough");
}

void HtmlEditor::formatAlignLeft()
{
    execCommand("justifyLeft");
}

void HtmlEditor::formatAlignCenter()
{
    execCommand("justifyCenter");
}

void HtmlEditor::formatAlignRight()
{
    execCommand("justifyRight");
}

void HtmlEditor::formatAlignJustify()
{
    execCommand("justifyFull");
}

void HtmlEditor::formatIncreaseIndent()
{
    execCommand("indent");
}

void HtmlEditor::formatDecreaseIndent()
{
    execCommand("outdent");
}

void HtmlEditor::formatNumberedList()
{
    execCommand("insertOrderedList");
}

void HtmlEditor::formatBulletedList()
{
    execCommand("insertUnorderedList");
}

void HtmlEditor::formatFontName()
{
    QStringList families = QFontDatabase().families();
    bool ok = false;
    QString family = QInputDialog::getItem(this, tr("Font"), tr("Select font:"),
                                           families, 0, false, &ok);

    if (ok)
        execCommand("fontName", family);
}

void HtmlEditor::formatFontSize()
{
    bool ok = false;
    int size = QInputDialog::getInt(this, tr("Font Size"), tr("Size in points:"), 48, 0, 1000, 1, &ok);
    if (ok) {
        QWebFrame *frame = ui->webView->page()->mainFrame();
        QString js;
        if (size)
            js = QString("setFontSize('%1pt')").arg(size);
        else
            js = QString("setFontSize('')");
        frame->evaluateJavaScript(js);
    }
}

void HtmlEditor::formatTextColor()
{
    QColor color = QColorDialog::getColor(Qt::black, this);
    if (color.isValid())
        execCommand("foreColor", color.name());
}

void HtmlEditor::formatBackgroundColor()
{
    QColor color = QColorDialog::getColor(Qt::white, this);
    if (color.isValid())
        execCommand("hiliteColor", color.name());
}

#define FOLLOW_ENABLE(a1, a2) a1->setEnabled(ui->webView->pageAction(a2)->isEnabled())
#define FOLLOW_CHECK(a1, a2) a1->setChecked(ui->webView->pageAction(a2)->isChecked())

void HtmlEditor::adjustActions()
{
    FOLLOW_ENABLE(ui->actionEditUndo, QWebPage::Undo);
    FOLLOW_ENABLE(ui->actionEditRedo, QWebPage::Redo);
    FOLLOW_ENABLE(ui->actionEditCut, QWebPage::Cut);
    FOLLOW_ENABLE(ui->actionEditCopy, QWebPage::Copy);
    FOLLOW_ENABLE(ui->actionEditPaste, QWebPage::Paste);
    FOLLOW_CHECK(ui->actionFormatBold, QWebPage::ToggleBold);
    FOLLOW_CHECK(ui->actionFormatItalic, QWebPage::ToggleItalic);
    FOLLOW_CHECK(ui->actionFormatUnderline, QWebPage::ToggleUnderline);

    ui->actionFormatStrikethrough->setChecked(queryCommandState("strikeThrough"));
    ui->actionFormatNumberedList->setChecked(queryCommandState("insertOrderedList"));
    ui->actionFormatBulletedList->setChecked(queryCommandState("insertUnorderedList"));
}

void HtmlEditor::adjustSource()
{
    setWindowModified(true);
    sourceDirty = true;

    if (ui->tabWidget->currentIndex() == 1)
        changeTab(1);
}

void HtmlEditor::changeTab(int index)
{
    if (sourceDirty && (index == 1)) {
        QString content = ui->webView->page()->mainFrame()->toHtml();
        ui->plainTextEdit->setPlainText(content);
        sourceDirty = false;
    }
}

void HtmlEditor::openLink(const QUrl &url)
{
    QString msg = QString(tr("Open %1 ?")).arg(url.toString());
    if (QMessageBox::question(this, tr("Open link"), msg,
                              QMessageBox::Open | QMessageBox::Cancel) ==
            QMessageBox::Open)
        QDesktopServices::openUrl(url);
}

void HtmlEditor::on_actionTextOutline_triggered()
{
    QQuickView* view = new QQuickView(QUrl::fromLocalFile(qmlFilePath("text_outline.qml")));
    view->setTitle(tr("Text Outline"));
    view->setColor(palette().window().color());
    connect(view->rootObject(), SIGNAL(accepted(QString)), SLOT(formatTextOutline(QString)));
    connect(view->engine(), SIGNAL(quit()), view, SLOT(close()));
    view->show();
}

void HtmlEditor::formatTextOutline(const QString& outline)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("formatTextOutline('%1')").arg(outline);
    frame->evaluateJavaScript(js);
}

void HtmlEditor::on_actionTextShadow_triggered()
{
    QQuickView* view = new QQuickView(QUrl::fromLocalFile(qmlFilePath("text_shadow.qml")));
    view->setTitle(tr("Text Shadow"));
    view->setColor(palette().window().color());
    connect(view->rootObject(), SIGNAL(accepted(QString)), SLOT(formatTextShadow(QString)));
    connect(view->engine(), SIGNAL(quit()), view, SLOT(close()));
    view->show();
}

void HtmlEditor::formatTextShadow(const QString& outline)
{
    QWebFrame *frame = ui->webView->page()->mainFrame();
    QString js = QString("formatTextShadow('%1')").arg(outline);
    frame->evaluateJavaScript(js);
}

void HtmlEditor::changeZoom(int percent)
{
    ui->actionZoomOut->setEnabled(percent > 25);
    ui->actionZoomIn->setEnabled(percent < 400);
    qreal factor = static_cast<qreal>(percent) / 100;
    ui->webView->setZoomFactor(factor);

    zoomLabel->setText(QString("%1% ").arg(percent));
    zoomSlider->setValue(percent);
}

void HtmlEditor::closeEvent(QCloseEvent *e)
{
    if (maybeSave()) {
        e->accept();
        emit closed();
    } else {
        e->ignore();
    }
}

bool HtmlEditor::load(const QString &f)
{
    if (!QFile::exists(f))
        return false;
    QFile file(f);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    ui->webView->setContent(data, "text/html");
    ui->webView->page()->setContentEditable(true);
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(ui->webView, SIGNAL(linkClicked(QUrl)), SLOT(openLink(QUrl)));
    const QString& html = ui->webView->page()->mainFrame()->toHtml();
    ui->actionFormatFontSize->setEnabled(html.contains("setFontSize"));
    ui->actionTextOutline->setEnabled(html.contains("formatTextOutline"));
    ui->actionTextShadow->setEnabled(html.contains("formatTextShadow"));
    setCurrentFileName(f);
    return true;
}

void HtmlEditor::resizeWebView(int w, int h)
{
    int widthDelta = width() - ui->webView->width();
    int heightDelta = height() - ui->webView->height();
    resize(w + widthDelta, h + heightDelta);
}

void HtmlEditor::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;

    QString shownName;
    if (fileName.isEmpty())
        shownName = "untitled";
    else
        shownName = QFileInfo(fileName).fileName();

    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("HTML Editor")));
    setWindowModified(false);

    bool allowSave = true;
    if (fileName.isEmpty() || fileName.startsWith(QLatin1String(":/")))
        allowSave = false;
    ui->actionFileSave->setEnabled(allowSave);
}
