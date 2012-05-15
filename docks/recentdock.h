#ifndef RECENTDOCK_H
#define RECENTDOCK_H

#include <QDockWidget>
#include <QSettings>

namespace Ui {
    class RecentDock;
}

class QListWidget;
class QListWidgetItem;

class RecentDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit RecentDock(QWidget *parent = 0);
    ~RecentDock();
    QListWidget* listWidget() const;

signals:
    void itemActivated(const QString& url);

public slots:
    void add(const QString&);

private:
    Ui::RecentDock *ui;
    QSettings m_settings;
    QStringList m_recent;

private slots:
    void on_listWidget_itemActivated(QListWidgetItem* i);
};

#endif // RECENTDOCK_H
