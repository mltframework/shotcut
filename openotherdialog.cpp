#include "openotherdialog.h"
#include "ui_openotherdialog.h"
#include "mltcontroller.h"
#include <Mlt.h>
#include <QtGui>

enum {
    NetworkTabIndex = 0,
    DeckLinkTabIndex,
    ColorTabIndex,
    NoiseTabIndex,
    IsingTabIndex,
    LissajousTabIndex,
    PlasmaTabIndex
};

OpenOtherDialog::OpenOtherDialog(Mlt::Controller *mc, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenOtherDialog),
    mlt(mc)
{
    ui->setupUi(this);
 
    // build the presets combo
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    if (dir.cd("presets") && dir.cd("producer")) {
        ui->presetCombo->addItems(dir.entryList(QDir::Files));
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Executable);
        foreach (QString s, entries) {
            if (dir.cd(s)) {
                QStringList entries2 = dir.entryList(QDir::Files | QDir::Readable);
                foreach (QString s2, entries2) {
                    ui->presetCombo->addItem(s2, s); // userData contains the producer name
                }
                dir.cdUp();
            }
        }
    }
    
    // hide tabs for unavailable producers
    if (!mlt->repository()->producers()->get_data("avformat"))
        ui->methodTabWidget->removeTab(NetworkTabIndex);
    if (!mlt->repository()->producers()->get_data("decklink"))
        ui->methodTabWidget->removeTab(DeckLinkTabIndex);
    if (!mlt->repository()->producers()->get_data("color"))
        ui->methodTabWidget->removeTab(ColorTabIndex);
    if (!mlt->repository()->producers()->get_data("frei0r.ising0r"))
        ui->methodTabWidget->removeTab(IsingTabIndex);
    if (!mlt->repository()->producers()->get_data("frei0r.lissajous0r"))
        ui->methodTabWidget->removeTab(LissajousTabIndex);
    if (!mlt->repository()->producers()->get_data("frei0r.plasma"))
        ui->methodTabWidget->removeTab(PlasmaTabIndex);
}

OpenOtherDialog::~OpenOtherDialog()
{
    delete ui;
}

QString OpenOtherDialog::producerName() const
{
    if (ui->methodTabWidget->currentWidget()->objectName() == "networkTab")
        return "avformat";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "decklinkTab")
        return "decklink";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "colorTab")
        return "color";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "noiseTab")
        return "noise";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "isingTab")
        return "frei0r.ising0r";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "lissajousTab")
        return "frei0r.lissajous0r";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "plasmaTab")
        return "frei0r.plasma";
    else
        return "color";
}

QString OpenOtherDialog::URL() const
{
    if (ui->methodTabWidget->currentWidget()->objectName() == "networkTab")
        return ui->urlLineEdit->text();
    else if (ui->methodTabWidget->currentWidget()->objectName() == "decklinkTab")
        return QString("decklink:%1").arg(ui->decklinkCardSpinner->value());
    else if (ui->methodTabWidget->currentWidget()->objectName() == "colorTab")
        return "color:";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "noiseTab")
        return "noise:";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "isingTab")
        return "frei0r.ising0r:";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "lissajousTab")
        return "frei0r.lissajous0r:";
    else if (ui->methodTabWidget->currentWidget()->objectName() == "plasmaTab")
        return "frei0r.plasma:";
    else
        return "color:";
}

Mlt::Properties* OpenOtherDialog::mltProperties() const
{
    Mlt::Properties* props = 0;
    if (ui->methodTabWidget->currentWidget()->objectName() == "colorTab") {
        props = new Mlt::Properties;
        props->set("colour", ui->colorLabel->text().toAscii().constData());
    }
    else if (ui->methodTabWidget->currentWidget()->objectName() == "isingTab") {
        props = new Mlt::Properties;
        props->set("Temperature", ui->tempSpinner->text().toAscii().constData());
        props->set("Border Growth", ui->borderGrowthSpinner->text().toAscii().constData());
        props->set("Spontaneous Growth", ui->spontGrowthSpinner->text().toAscii().constData());
    }
    else if (ui->methodTabWidget->currentWidget()->objectName() == "lissajousTab") {
        props = new Mlt::Properties;
        props->set("ratiox", ui->xratioSpinner->text().toAscii().constData());
        props->set("ratioy", ui->yratioSpinner->text().toAscii().constData());
    }
    else if (ui->methodTabWidget->currentWidget()->objectName() == "plasmaTab") {
        props = new Mlt::Properties;
        props->set("1_speed", ui->speed1Spinner->text().toAscii().constData());
        props->set("2_speed", ui->speed2Spinner->text().toAscii().constData());
        props->set("3_speed", ui->speed3Spinner->text().toAscii().constData());
        props->set("4_speed", ui->speed4Spinner->text().toAscii().constData());
        props->set("1_move", ui->move1Spinner->text().toAscii().constData());
        props->set("2_move", ui->move2Spinner->text().toAscii().constData());
    }
    return props;
}


void OpenOtherDialog::on_savePresetButton_clicked()
{
    QString preset = QInputDialog::getText(this, tr("Save Preset"), tr("Name:") );
    if (!preset.isNull()) {
        QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
        QString producer = producerName();
        Mlt::Properties* props = mltProperties();

        if (!dir.exists())
            dir.mkpath(dir.path());
        if (!dir.cd("presets")) {
            if (dir.mkdir("presets"))
                dir.cd("presets");
        }
        if (!dir.cd("producer")) {
            if (dir.mkdir("producer"))
                dir.cd("producer");
        }
        if (!dir.cd(producer)) {
            if (dir.mkdir(producer))
                dir.cd(producer);
        }
        if (!props)
            props = new Mlt::Properties;
        props->set("URL", URL().toUtf8().constData());
        props->save(dir.filePath(preset).toUtf8().constData());
    }
}

void OpenOtherDialog::on_colorButton_clicked()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    if (dialog.exec() == QDialog::Accepted) {
        ui->colorLabel->setText(QString().sprintf("#%02X%02X%02X%02X",
                                                  qAlpha(dialog.currentColor().rgba()),
                                                  qRed(dialog.currentColor().rgba()),
                                                  qGreen(dialog.currentColor().rgba()),
                                                  qBlue(dialog.currentColor().rgba())
                                                  ));
        ui->colorLabel->setStyleSheet(QString("background-color: %1").arg(dialog.currentColor().name()));
    }
}

///////////////// Ising //////////////////////

void OpenOtherDialog::on_tempDial_valueChanged(int value)
{
    ui->tempSpinner->setValue(value/100.0);
}

void OpenOtherDialog::on_tempSpinner_valueChanged(double value)
{
    ui->tempDial->setValue(value * 100);
}

void OpenOtherDialog::on_borderGrowthDial_valueChanged(int value)
{
    ui->borderGrowthSpinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_borderGrowthSpinner_valueChanged(double value)
{
    ui->borderGrowthDial->setValue(value * 100);
}

void OpenOtherDialog::on_spontGrowthDial_valueChanged(int value)
{
    ui->spontGrowthSpinner->setValue(value/100.0);
}

void OpenOtherDialog::on_spontGrowthSpinner_valueChanged(double value)
{
    ui->spontGrowthDial->setValue(value * 100);
}

/////////////////// Lissajous ///////////////////////////////

void OpenOtherDialog::on_xratioDial_valueChanged(int value)
{
    ui->xratioSpinner->setValue(value/100.0);
}

void OpenOtherDialog::on_xratioSpinner_valueChanged(double value)
{
    ui->xratioDial->setValue(value * 100);
}

void OpenOtherDialog::on_yratioDial_valueChanged(int value)
{
    ui->yratioSpinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_yratioSpinner_valueChanged(double value)
{
    ui->yratioDial->setValue(value * 100);
}

///////////////////// Plasma /////////////////////

void OpenOtherDialog::on_speed1Dial_valueChanged(int value)
{
    ui->speed1Spinner->setValue(value/100.0);
}

void OpenOtherDialog::on_speed1Spinner_valueChanged(double value)
{
    ui->speed1Dial->setValue(value * 100);
}

void OpenOtherDialog::on_speed2Dial_valueChanged(int value)
{
    ui->speed2Spinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_speed2Spinner_valueChanged(double value)
{
    ui->speed2Dial->setValue(value * 100);
}

void OpenOtherDialog::on_speed3Dial_valueChanged(int value)
{
    ui->speed3Spinner->setValue(value/100.0);
}

void OpenOtherDialog::on_speed3Spinner_valueChanged(double value)
{
    ui->speed3Dial->setValue(value * 100);
}

void OpenOtherDialog::on_speed4Dial_valueChanged(int value)
{
    ui->speed4Spinner->setValue(value/100.0);
}

void OpenOtherDialog::on_speed4Spinner_valueChanged(double value)
{
    ui->speed4Dial->setValue(value * 100);
}

void OpenOtherDialog::on_move1Dial_valueChanged(int value)
{
    ui->move1Spinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_move1Spinner_valueChanged(double value)
{
    ui->move1Dial->setValue(value * 100);
}

void OpenOtherDialog::on_move2Dial_valueChanged(int value)
{
    ui->move2Spinner->setValue(value/100.0);    
}

void OpenOtherDialog::on_move2Spinner_valueChanged(double value)
{
    ui->move2Dial->setValue(value * 100);
}

void OpenOtherDialog::load(QString& producer, Mlt::Properties& p)
{
    for (int i = 0; i < ui->methodTabWidget->count(); i++) {
        if (ui->methodTabWidget->widget(i)->objectName() == "networkTab"
                && producer == "avformat") {
            ui->methodTabWidget->setCurrentIndex(i);
            ui->urlLineEdit->setText(p.get("URL"));
        }
        else if (ui->methodTabWidget->widget(i)->objectName() == "decklinkTab"
                && producer == "decklink") {
            ui->methodTabWidget->setCurrentIndex(i);
            QString s(p.get("URL"));
            ui->decklinkCardSpinner->setValue(s.mid(s.indexOf(':') + 1).toInt());
        }
        else if (ui->methodTabWidget->widget(i)->objectName() == "colorTab"
                && producer == "color") {
            ui->methodTabWidget->setCurrentIndex(i);
            ui->colorLabel->setText(p.get("colour"));
            ui->colorLabel->setStyleSheet(QString("background-color: %1")
                .arg(QString(p.get("colour")).replace(0, 3, "#")));
        }
        else if (ui->methodTabWidget->widget(i)->objectName() == "noiseTab"
                && producer == "noise") {
            ui->methodTabWidget->setCurrentIndex(i);
        }
        else if (ui->methodTabWidget->widget(i)->objectName() == "isingTab"
                && producer == "frei0r.ising0r") {
            ui->methodTabWidget->setCurrentIndex(i);
            ui->tempSpinner->setValue(p.get_double("Temperature"));
            ui->borderGrowthSpinner->setValue(p.get_double("Border Growth"));
            ui->spontGrowthSpinner->setValue(p.get_double("Spontaneous Growth"));
        }
        else if (ui->methodTabWidget->widget(i)->objectName() == "lissajousTab"
                && producer == "frei0r.lissajous0r") {
            ui->methodTabWidget->setCurrentIndex(i);
            ui->xratioSpinner->setValue(p.get_double("ratiox"));
            ui->yratioSpinner->setValue(p.get_double("ratioy"));
        }
        else if (ui->methodTabWidget->widget(i)->objectName() == "plasmaTab"
                && producer == "frei0r.plasma") {
            ui->methodTabWidget->setCurrentIndex(i);
            ui->speed1Spinner->setValue(p.get_double("1_speed"));
            ui->speed2Spinner->setValue(p.get_double("2_speed"));
            ui->speed3Spinner->setValue(p.get_double("3_speed"));
            ui->speed4Spinner->setValue(p.get_double("4_speed"));
            ui->move1Spinner->setValue(p.get_double("1_move"));
            ui->move2Spinner->setValue(p.get_double("1_move"));
        }
    }
}

void OpenOtherDialog::on_presetCombo_activated(int index)
{
    QString producer = ui->presetCombo->itemData(index).toString();
    QString preset = ui->presetCombo->itemText(index);
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    Mlt::Properties p;
    
    if (!dir.cd("presets") || !dir.cd("producer") || !dir.cd(producer))
        return;
    p.load(dir.filePath(preset).toUtf8().constData());
    load(producer, p);
 }
