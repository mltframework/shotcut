#ifndef OPENOTHERDIALOG_H
#define OPENOTHERDIALOG_H

#include <QDialog>

namespace Ui {
    class OpenOtherDialog;
}
namespace Mlt {
    class Controller;
    class Properties;
}

class OpenOtherDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenOtherDialog(Mlt::Controller*, QWidget *parent = 0);
    ~OpenOtherDialog();
    
    QString producerName() const;
    QString URL() const;
    Mlt::Properties* mltProperties() const;
    void load(QString& producer, Mlt::Properties& p);

private slots:
    void on_colorButton_clicked();
    
    void on_tempDial_valueChanged(int value);
    void on_tempSpinner_valueChanged(double arg1);
    void on_borderGrowthDial_valueChanged(int value);
    void on_borderGrowthSpinner_valueChanged(double arg1);
    void on_spontGrowthDial_valueChanged(int value);
    void on_spontGrowthSpinner_valueChanged(double arg1);
    
    void on_xratioDial_valueChanged(int value);
    void on_xratioSpinner_valueChanged(double arg1);
    void on_yratioDial_valueChanged(int value);
    void on_yratioSpinner_valueChanged(double arg1);
    
    void on_speed1Dial_valueChanged(int value);
    void on_speed1Spinner_valueChanged(double arg1);
    void on_speed2Dial_valueChanged(int value);
    void on_speed2Spinner_valueChanged(double arg1);
    void on_speed3Dial_valueChanged(int value);
    void on_speed3Spinner_valueChanged(double arg1);
    void on_speed4Dial_valueChanged(int value);
    void on_speed4Spinner_valueChanged(double arg1);
    void on_move1Dial_valueChanged(int value);
    void on_move1Spinner_valueChanged(double arg1);
    void on_move2Dial_valueChanged(int value);
    void on_move2Spinner_valueChanged(double arg1);

    void on_savePresetButton_clicked();
    
    void on_presetCombo_activated(int index);
    
private:
    Ui::OpenOtherDialog *ui;
    Mlt::Controller *mlt;
};

#endif // OPENOTHERDIALOG_H
