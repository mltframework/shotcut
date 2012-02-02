#ifndef LISSAJOUSWIDGET_H
#define LISSAJOUSWIDGET_H

#include <QWidget>
#include "abstractproducerwidget.h"

namespace Ui {
    class LissajousWidget;
}

class LissajousWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit LissajousWidget(QWidget *parent = 0);
    ~LissajousWidget();

    // AbstractProducerWidget overrides
    QString producerName() const
        { return "frei0r.lissajous0r"; }
    Mlt::Properties* mltProperties();
    void load(Mlt::Properties&);

private slots:
    void on_xratioDial_valueChanged(int value);
    void on_xratioSpinner_valueChanged(double arg1);
    void on_yratioDial_valueChanged(int value);
    void on_yratioSpinner_valueChanged(double arg1);
    
private:
    Ui::LissajousWidget *ui;
};

#endif // LISSAJOUSWIDGET_H
