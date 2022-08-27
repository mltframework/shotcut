/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "actionsdialog.h"

#include "actions.h"
#include "widgets/statuslabelwidget.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QAction>
#include <QPushButton>
#include <QKeyEvent>

static const unsigned int editorWidth = 180;

class ShortcutEditor : public QWidget
{
    Q_OBJECT

public:
    ShortcutEditor(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumWidth(editorWidth);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        seqEdit = new QKeySequenceEdit();
        layout->addWidget(seqEdit);

        QToolButton *defaultButton = new QToolButton();
        defaultButton->setIcon(QIcon::fromTheme("edit-undo",
                                                QIcon(":/icons/oxygen/32x32/actions/edit-undo.png")));
        defaultButton->setText(tr("Set to default"));
        defaultButton->setToolTip(tr("Set to default"));
        connect(defaultButton, &QToolButton::clicked, this, [&]() {
            seqEdit->setKeySequence(defaultSeq);
        });
        layout->addWidget(defaultButton);

        QToolButton *clearButton = new QToolButton();
        clearButton->setIcon(QIcon::fromTheme("edit-clear",
                                              QIcon(":/icons/oxygen/32x32/actions/edit-clear.png")));
        clearButton->setText(tr("Clear shortcut"));
        clearButton->setToolTip(tr("Clear shortcut"));
        connect(clearButton, &QToolButton::clicked, this, [&]() {
            seqEdit->clear();
        });
        layout->addWidget(clearButton);

        setLayout(layout);
    }

    ~ShortcutEditor() = default;

    QKeySequenceEdit *seqEdit;
    QKeySequence defaultSeq;
};

class ShortcutItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ShortcutItemDelegate(QWidget *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
    {
        if (index.column() == ActionsModel::COLUMN_SEQUENCE1 ||
                (index.column() == ActionsModel::COLUMN_SEQUENCE2
                 && !index.data(ActionsModel::HardKeyRole).isValid() )) {
            // Hard key shortcuts are in column 2 and are not editable.
            return new ShortcutEditor(parent);
        }
        return nullptr;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        ShortcutEditor *widget = dynamic_cast<ShortcutEditor *>(editor);
        if (widget) {
            widget->seqEdit->setKeySequence(index.data(Qt::EditRole).value<QKeySequence>());
            widget->defaultSeq = index.data(ActionsModel::DefaultKeyRole).value<QKeySequence>();
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        QKeySequence newSeq = static_cast<ShortcutEditor *>(editor)->seqEdit->keySequence();
        model->setData(index, newSeq);
    }
};

class KeyPressFilter : public QObject
{
    Q_OBJECT
public:
    KeyPressFilter(QObject *parent = 0) : QObject(parent) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress) {
            auto keyEvent = static_cast<QKeyEvent *>(event);
            if (!keyEvent->modifiers() && (keyEvent->key() == Qt::Key_Return
                                           || keyEvent->key() == Qt::Key_Enter)) {
                auto window = static_cast<QWidget *>(parent());
                window->close();
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

class PrivateTreeView : public QTreeView
{
    Q_OBJECT
public:
    PrivateTreeView(QWidget *parent = nullptr) : QTreeView(parent) {}

    virtual bool edit(const QModelIndex &index, QAbstractItemView::EditTrigger trigger,
                      QEvent *event) override
    {
        bool editInProgress = QTreeView::edit(index, trigger, event);
        if (editInProgress)
            emit editStarted();
        return editInProgress;
    }

signals:
    void editStarted();
};

// Include this so that ShortcutItemDelegate can be declared in the source file.
#include "actionsdialog.moc"

ActionsDialog::ActionsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Actions and Shortcuts"));
    setSizeGripEnabled(true) ;

    QVBoxLayout *vlayout = new QVBoxLayout();

    // Search Bar
    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText(tr("search"));
    connect(m_searchField, &QLineEdit::textChanged, this, [&](const QString & text) {
        if (m_proxyModel) {
            m_proxyModel->setFilterRegExp(QRegExp(text, Qt::CaseInsensitive, QRegExp::FixedString));
        }
    });
    connect(m_searchField, &QLineEdit::returnPressed, this, [&] {
        m_table->setFocus();
        m_table->setCurrentIndex(m_proxyModel->index(0, 0));
    });
    searchLayout->addWidget(m_searchField);
    QToolButton *clearSearchButton = new QToolButton(this);
    clearSearchButton->setIcon(QIcon::fromTheme("edit-clear",
                                                QIcon(":/icons/oxygen/32x32/actions/edit-clear.png")));
    clearSearchButton->setMaximumSize(22, 22);
    clearSearchButton->setToolTip(tr("Clear search"));
    clearSearchButton->setAutoRaise(true);
    connect(clearSearchButton, &QAbstractButton::clicked, m_searchField, &QLineEdit::clear);
    searchLayout->addWidget(clearSearchButton);
    vlayout->addLayout(searchLayout);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(&m_model);
    m_proxyModel->setFilterKeyColumn(-1);

    // List
    m_table = new PrivateTreeView();
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setItemsExpandable(false);
    m_table->setRootIsDecorated(false);
    m_table->setUniformRowHeights(true);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    m_table->setItemDelegateForColumn(1, new ShortcutItemDelegate(this));
    m_table->setItemDelegateForColumn(2, new ShortcutItemDelegate(this));
    m_table->setModel(m_proxyModel);
    m_table->setWordWrap(false);
    m_table->setSortingEnabled(true);
    m_table->header()->setStretchLastSection(false);
    m_table->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->header()->setSectionResizeMode(1, QHeaderView::QHeaderView::Fixed);
    m_table->header()->setSectionResizeMode(2, QHeaderView::QHeaderView::Fixed);
    m_table->header()->resizeSection(1, editorWidth);
    m_table->header()->resizeSection(2, editorWidth);
    m_table->sortByColumn(ActionsModel::COLUMN_ACTION, Qt::AscendingOrder);
    m_table->installEventFilter(new KeyPressFilter(this));
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
    this, [&](const QItemSelection & selected, const QItemSelection & deselected) {
        m_status->showText(tr("Click on the selected shortcut to show the editor"), 5, nullptr,
                           QPalette::AlternateBase);
    });
    connect(m_table, &PrivateTreeView::editStarted, this, [&]() {
        m_status->showText(tr("Click on the shortcut editor to capture key presses"), 5, nullptr,
                           QPalette::AlternateBase);
    });
    vlayout->addWidget(m_table);

    QHBoxLayout *hlayout = new QHBoxLayout();
    m_status = new StatusLabelWidget();
    connect(&m_model, &ActionsModel::editError, this, [&](const QString & message) {
        m_status->showText(message, 5, nullptr);
    });
    hlayout->addWidget(m_status);

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    hlayout->addWidget(buttonBox);

    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    connect(m_table, &QAbstractItemView::activated, this, [&](const QModelIndex & index) {
        auto action = m_model.action(m_proxyModel->mapToSource(index));
        if (action && action->isEnabled()) {
            action->trigger();
        }
    });

    int tableWidth = 38;
    for (int i = 0; i < m_table->model()->columnCount(); i++) {
        tableWidth += m_table->columnWidth(i);
    }
    resize(tableWidth, 600);
}

void ActionsDialog::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    // Reset the dialog when hidden since it is no longer destroyed.
    m_searchField->setFocus();
    m_searchField->clear();
    m_table->clearSelection();
}
