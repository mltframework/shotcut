#include "docks/recentdock.h"
#include "ui_recentdock.h"
#include <QSettings>
#include <QFileInfo>

static const int MaxItems = 50;

RecentDock::RecentDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::RecentDock)
{
    ui->setupUi(this);
    toggleViewAction()->setIcon(QIcon::fromTheme("document-open-recent", windowIcon()));
    m_recent = m_settings.value("recent").toStringList();
    add(QString());
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
    if (!s.isEmpty()) {
        m_recent.removeOne(s);
        m_recent.prepend(s);
        while (m_recent.count() > MaxItems)
            m_recent.removeLast();
    }
    ui->listWidget->clear();
    foreach (QString s, m_recent) {
        QString name = s;
        if (s.startsWith('/'))
            // Use basename instead.
            name = QFileInfo(s).fileName();
        QListWidgetItem* item  = new QListWidgetItem(name);
        item->setToolTip(s);
        ui->listWidget->addItem(item);
    }
    m_settings.setValue("recent", m_recent);
}

void RecentDock::on_listWidget_itemActivated(QListWidgetItem* i)
{
    emit itemActivated(i->toolTip());
}
