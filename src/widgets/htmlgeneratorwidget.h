/*
 * Copyright (c) 2025 Meltytech, LLC
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

#ifndef HTMLGENERATORWIDGET_H
#define HTMLGENERATORWIDGET_H

#include "abstractproducerwidget.h"

#include <QWidget>

namespace Ui {
class HtmlGeneratorWidget;
}

class HtmlGeneratorWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    static const char *kColorProperty;
    static const char *kCssProperty;
    static const char *kBodyProperty;
    static const char *kJavaScriptProperty;
    static const char *kLine1Property;
    static const char *kLine2Property;
    static const char *kLine3Property;

    explicit HtmlGeneratorWidget(QWidget *parent = 0);
    ~HtmlGeneratorWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &) override;
    Mlt::Properties getPreset() const override;
    void loadPreset(Mlt::Properties &) override;
    void setProducer(Mlt::Producer *p) override;

signals:
    void producerChanged(Mlt::Producer *);

private slots:
    void on_colorButton_clicked();
    void on_preset_selected(void *p);
    void on_preset_saveClicked();

    void on_imageButton_clicked();

    void on_pushButton_clicked();

    void on_videoButton_clicked();

    void on_cssToggleButton_toggled(bool checked);

    void on_bodyToggleButton_toggled(bool checked);

    void on_javascriptToggleButton_toggled(bool checked);

    void on_bodyTextEdit_textChanged();

    void on_presetIconView_itemClicked(class QListWidgetItem *item);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QString generateHtml() const;
    void updateTextSectionVisibility();
    void populatePresetIconView();
    QString findPresetIconPath(const QString &presetName);
    void setupIconAnimation(class QListWidgetItem *item, const QString &iconPath);
    Ui::HtmlGeneratorWidget *ui;
    QList<class QMovie *> m_iconMovies;
    class QListWidgetItem *m_lastHoveredItem = nullptr;
};

#endif // TEXTPRODUCERWIDGET_H
