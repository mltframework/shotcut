#include "docks/recentdock.h"
#include "ui_recentdock.h"
#include <QSettings>

static const int MaxItems = 50;

RecentDock::RecentDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::RecentDock)
{
    ui->setupUi(this);
    m_recent = m_settings.value("recent").toStringList();
    ui->listWidget->addItems(m_recent);
}

RecentDock::~RecentDock()
{
    delete ui;
}

QListWidget* RecentDock::listWidget() const
{
    return ui->listWidget;
}

void RecentDock::add(const QString &s)
{
    m_recent.removeOne(s);
    m_recent.prepend(s);
    while (m_recent.count() > MaxItems)
        m_recent.removeLast();
    ui->listWidget->clear();
    ui->listWidget->addItems(m_recent);
    m_settings.setValue("recent", m_recent);
}

void RecentDock::on_listWidget_itemActivated(QListWidgetItem* i)
{
    emit itemActivated(i->text());
}
