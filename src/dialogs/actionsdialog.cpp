/*
 * Copyright (c) 20222-2024 Meltytech, LLC
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

#include "widgets/statuslabelwidget.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
// 定义快捷键编辑器的宽度常量
static const unsigned int editorWidth = 180;
// 自定义快捷键编辑器部件类，用于编辑快捷键序列
class ShortcutEditor : public QWidget
{
    Q_OBJECT    // 启用 Qt 的元对象系统，支持信号与槽机制

public:
// 构造函数，parent 为父部件
    ShortcutEditor(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        // 设置部件最小宽度
        setMinimumWidth(editorWidth);
        // 设置尺寸策略：水平方向扩展，垂直方向固定
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // 创建水平布局，用于排列子部件
        QHBoxLayout *layout = new QHBoxLayout(this);
        // 设置布局的外边距为 0
        layout->setContentsMargins(0, 0, 0, 0);
        // 设置布局内控件间距为 0
        layout->setSpacing(0);
        // 创建用于编辑快捷键序列的部件
        seqEdit = new QKeySequenceEdit();
        // 将其添加到布局
        layout->addWidget(seqEdit);
        
        // 【创建“应用”工具按钮】
        QToolButton *applyButton = new QToolButton();
        // 设置按钮图标，优先从系统主题获取，否则用指定资源文件图标
        applyButton->setIcon(
            QIcon::fromTheme("dialog-ok", QIcon(":/icons/oxygen/32x32/actions/dialog-ok.png")));
        // 设置按钮显示文本（翻译支持）
        applyButton->setText(tr("Apply"));
        // 设置按钮提示文本（翻译支持）
        applyButton->setToolTip(tr("Apply"));
        // 连接按钮点击信号到 lambda 表达式，触发 applied 信号
        connect(applyButton, &QToolButton::clicked, this, [this]() { emit applied(); });
        // 将按钮添加到布局
        layout->addWidget(applyButton);

       // 【创建“恢复默认”工具按钮】
        QToolButton *defaultButton = new QToolButton();
        // 设置按钮图标，优先从系统主题获取，否则用指定资源文件图标
        defaultButton->setIcon(
            QIcon::fromTheme("edit-undo", QIcon(":/icons/oxygen/32x32/actions/edit-undo.png")));
        // 设置按钮显示文本（翻译支持）
        defaultButton->setText(tr("Set to default"));
        // 设置按钮提示文本（翻译支持）
        defaultButton->setToolTip(tr("Set to default"));
        // 连接按钮点击信号到 lambda 表达式，将编辑器内容设为默认序列
        connect(defaultButton, &QToolButton::clicked, this, [&]() {
            seqEdit->setKeySequence(defaultSeq);
        });
        // 将按钮添加到布局
        layout->addWidget(defaultButton);

       // 【创建“清除快捷键”工具按钮】
        QToolButton *clearButton = new QToolButton();
        // 设置按钮图标，优先从系统主题获取，否则用指定资源文件图标
        clearButton->setIcon(
            QIcon::fromTheme("edit-clear", QIcon(":/icons/oxygen/32x32/actions/edit-clear.png")));
        // 设置按钮显示文本（翻译支持）
        clearButton->setText(tr("Clear shortcut"));
        // 设置按钮提示文本（翻译支持）
        clearButton->setToolTip(tr("Clear shortcut"));
        // 连接按钮点击信号到 lambda 表达式，清除编辑器内容
        connect(clearButton, &QToolButton::clicked, this, [&]() { seqEdit->clear(); });
        // 将按钮添加到布局
        layout->addWidget(clearButton);
        // 将布局设置给当前部件
        setLayout(layout);
        // 延迟调用 seqEdit 的 setFocus 方法，确保获取焦点
        QMetaObject::invokeMethod(seqEdit, "setFocus", Qt::QueuedConnection);
    }
    // 析构函数，默认实现
    ~ShortcutEditor() = default;

    // 快捷键序列编辑器部件指针
    QKeySequenceEdit *seqEdit;
    // 默认的快捷键序列
    QKeySequence defaultSeq;

signals:
    // 当“应用”按钮点击时发出的信号，通知外部保存编辑内容
    void applied();
};

// 【自定义项委托类】：用于在 QTreeView 中编辑快捷键相关列
class ShortcutItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT  // 启用 Qt 元对象系统

public:
    // 构造函数，parent 为父对象
    ShortcutItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    // 【创建编辑器部件】：根据列索引判断是否创建 ShortcutEditor
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
    {
        // 判断是否是可编辑的快捷键列（列 1 或列 2 且不是硬编码快捷键）
        if (index.column() == ActionsModel::COLUMN_SEQUENCE1
            || (index.column() == ActionsModel::COLUMN_SEQUENCE2
                && !index.data(ActionsModel::HardKeyRole).isValid())) {
            // 创建自定义的 ShortcutEditor 作为编辑器
            m_currentEditor = new ShortcutEditor(parent);
            // 连接编辑器的 applied 信号到 lambda 表达式，触发保存逻辑
            connect(m_currentEditor, &ShortcutEditor::applied, this, [this]() {
                // 获取父部件（ActionsDialog）并调用其保存当前编辑器内容的方法
                auto dialog = static_cast<ActionsDialog *>(QObject::parent());
                dialog->saveCurrentEditor();
            });
            // 设置编辑器获取焦点
            m_currentEditor->setFocus();
            // 返回创建的编辑器部件
            return m_currentEditor;
        }
        // 其他列返回空（不创建编辑器）
        return nullptr;
    }
    // 销毁编辑器部件时，重置当前编辑器指针
    void destroyEditor(QWidget *editor, const QModelIndex &index) const
    {
        m_currentEditor = nullptr;
        // 调用父类销毁编辑器的逻辑
        QStyledItemDelegate::destroyEditor(editor, index);
    }

    // 将模型数据设置到编辑器部件
    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        // 将 editor 转换为 ShortcutEditor 类型
        ShortcutEditor *widget = dynamic_cast<ShortcutEditor *>(editor);
        if (widget) {
            // 从模型索引获取当前快捷键序列，设置到编辑器
            widget->seqEdit->setKeySequence(index.data(Qt::EditRole).value<QKeySequence>());
            // 从模型索引获取默认快捷键序列，保存到编辑器
            widget->defaultSeq = index.data(ActionsModel::DefaultKeyRole).value<QKeySequence>();
        }
    }
    // 将编辑器部件的数据设置回模型
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        // 获取编辑器中的新快捷键序列
        QKeySequence newSeq = static_cast<ShortcutEditor *>(editor)->seqEdit->keySequence();
        // 将新序列设置回模型对应索引位置
        model->setData(index, newSeq);
    }

    // 获取当前正在使用的编辑器部件指针
    ShortcutEditor *currentEditor() const { return m_currentEditor; }

private:
    // 当前正在使用的快捷键编辑器部件指针（可变，因为在 const 成员函数中可能修改）
    mutable ShortcutEditor *m_currentEditor = nullptr;
};

// 自定义事件过滤器类，处理按键事件（比如回车关闭窗口）
class KeyPressFilter : public QObject
{
    Q_OBJECT  // 启用 Qt 元对象系统
public:
    // 构造函数，parent 为父对象
    KeyPressFilter(QObject *parent = 0)
        : QObject(parent)
    {}

protected:
    // 重写事件过滤方法，处理特定按键事件
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        // 判断事件类型是否为按键按下
        if (event->type() == QEvent::KeyPress) {
            // 转换为 QKeyEvent 类型
            auto keyEvent = static_cast<QKeyEvent *>(event);
            // 判断是否是无修饰键的回车或确认键
            if (!keyEvent->modifiers()
                && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
                // 获取父部件（窗口）并关闭它
                auto window = static_cast<QWidget *>(parent());
                window->close();
            }
        }
        // 调用父类的事件过滤逻辑，继续传递事件
        return QObject::eventFilter(obj, event);
    }
};

// 自定义 QTreeView 子类，用于处理编辑相关的自定义逻辑
class PrivateTreeView : public QTreeView
{
    Q_OBJECT  // 启用 Qt 元对象系统

public:
    // 构造函数，parent 为父部件
    PrivateTreeView(QWidget *parent = nullptr)
        : QTreeView(parent)
    {}

    // 重写编辑方法，处理编辑状态相关逻辑
    virtual bool edit(const QModelIndex &index,
                      QAbstractItemView::EditTrigger trigger,
                      QEvent *event) override
    {
        // 调用父类的 edit 方法，获取是否开始编辑的结果
        bool editInProgress = QTreeView::edit(index, trigger, event);
        // 如果开始编辑，且触发方式是所有编辑触发，且是快捷键相关列
        if (editInProgress && trigger == QAbstractItemView::AllEditTriggers
            && (index.column() == ActionsModel::COLUMN_SEQUENCE1
                || index.column() == ActionsModel::COLUMN_SEQUENCE2)) {
            // 如果当前状态不是编辑状态，发出编辑被拒绝信号
            if (state() != QAbstractItemView::EditingState)
                emit editRejected();
        }
        // 返回编辑是否开始的结果
        return editInProgress;
    }

#ifdef Q_OS_MAC
    // macOS 平台下重写按键事件处理（处理回车、F2 等按键逻辑）
    virtual void keyPressEvent(QKeyEvent *event) override
    {
        // 判断按键是否是回车或确认键
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            // 发出当前索引被激活的信号
            emit activated(currentIndex());
        }
        // 判断按键是否是 F2
        else if (event->key() == Qt::Key_F2) {
            // 对当前索引执行编辑操作（模拟 EditKeyPressed 触发）
            edit(currentIndex(), QAbstractItemView::EditKeyPressed, event);
        } else {
            // 其他按键，调用父类的按键事件处理逻辑
            QAbstractItemView::keyPressEvent(event);
        }
    }
#endif

signals:
    // 当编辑被拒绝时发出的信号
    void editRejected();
};


// 自定义事件过滤器类，处理搜索栏的按键事件（上下键聚焦搜索结果）
class SearchKeyPressFilter : public QObject
{
    Q_OBJECT  // 启用 Qt 元对象系统

public:
    // 构造函数，parent 为父对象
    SearchKeyPressFilter(QObject *parent = 0)
        : QObject(parent)
    {}

protected:
    // 重写事件过滤方法，处理特定按键事件
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        // 判断事件类型是否为按键按下
        if (event->type() == QEvent::KeyPress) {
            // 转换为 QKeyEvent 类型
            auto keyEvent = static_cast<QKeyEvent *>(event);
            // 判断是否是上下方向键
            if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) {
                // 获取父部件（ActionsDialog）并调用其聚焦搜索结果的方法
                auto dialog = static_cast<ActionsDialog *>(parent());
                dialog->focusSearchResults();
                // 标记事件已处理
                event->accept();
            }
        }
        // 调用父类的事件过滤逻辑，继续传递事件
        return QObject::eventFilter(obj, event);
    }
};

// Include this so that ShortcutItemDelegate can be declared in the source file.
#include "actionsdialog.moc"

// ActionsDialog 类构造函数，parent 为父部件
ActionsDialog::ActionsDialog(QWidget *parent)
    : QDialog(parent)
{
    // 设置窗口标题（翻译支持）
    setWindowTitle(tr("Actions and Shortcuts"));
    // 启用窗口大小抓握（可拖动调整大小）
    setSizeGripEnabled(true);

    // 创建垂直布局，作为对话框主布局
    QVBoxLayout *vlayout = new QVBoxLayout();

    // 搜索栏布局
    QHBoxLayout *searchLayout = new QHBoxLayout();
    // 创建搜索输入框
    m_searchField = new QLineEdit(this);
    // 设置占位文本（翻译支持）
    m_searchField->setPlaceholderText(tr("search"));
    // 为搜索输入框安装事件过滤器（处理上下键等逻辑）
    m_searchField->installEventFilter(new SearchKeyPressFilter(this));
    // 连接文本变化信号到 lambda 表达式，实现搜索过滤
    connect(m_searchField, &QLineEdit::textChanged, this, [&](const QString &text) {
        if (m_proxyModel) {  // 如果代理模型存在
            // 设置过滤大小写不敏感
            m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
            // 设置过滤的固定字符串（即搜索内容）
            m_proxyModel->setFilterFixedString(text);
        }
    });
    // 连接回车按下信号到聚焦搜索结果的方法
    connect(m_searchField, &QLineEdit::returnPressed, this, &ActionsDialog::focusSearchResults);
    // 将搜索输入框添加到搜索布局
    searchLayout->addWidget(m_searchField);

    // 创建清除搜索内容的工具按钮
    QToolButton *clearSearchButton = new QToolButton(this);
    // 设置按钮图标，优先从系统主题获取，否则用指定资源文件图标
    clearSearchButton->setIcon(
        QIcon::fromTheme("edit-clear", QIcon(":/icons/oxygen/32x32/actions/edit-clear.png")));
    // 设置按钮最大尺寸
    clearSearchButton->setMaximumSize(22, 22);
    // 设置按钮提示文本（翻译支持）
    clearSearchButton->setToolTip(tr("Clear search"));
    // 设置按钮自动提升样式（外观更简洁）
    clearSearchButton->setAutoRaise(true);
    // 连接按钮点击信号到搜索输入框的清除方法
    connect(clearSearchButton, &QAbstractButton::clicked, m_searchField, &QLineEdit::clear);
    // 将清除按钮添加到搜索布局
    searchLayout->addWidget(clearSearchButton);
    // 将搜索布局添加到主垂直布局
    vlayout->addLayout(searchLayout);

   // 创建代理模型（用于搜索过滤等），父对象为当前对话框
    m_proxyModel = new QSortFilterProxyModel(this);
    // 设置代理模型的源模型为 m_model（ActionsModel 类型，代码中未完整展示 ）
    m_proxyModel->setSourceModel(&m_model);
    // 设置过滤的键列（-1 表示过滤所有列）
    m_proxyModel->setFilterKeyColumn(-1);

    // 创建自定义的树视图部件
    m_table = new PrivateTreeView();
    // 设置选择模式为单一选择
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    // 设置项不可展开
    m_table->setItemsExpandable(false);
    // 设置根项不显示装饰（如展开箭头等）
    m_table->setRootIsDecorated(false);
    // 设置行高均匀
    m_table->setUniformRowHeights(true);
    // 启用排序功能
    m_table->setSortingEnabled(true);
    // 设置编辑触发方式：选中点击或编辑键按下
    m_table->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    // 为列 1 和列 2 设置自定义项委托（处理快捷键编辑）
    m_table->setItemDelegateForColumn(1, new ShortcutItemDelegate(this));
    m_table->setItemDelegateForColumn(2, new ShortcutItemDelegate(this));
    // 设置模型为代理模型
    m_table->setModel(m_proxyModel);
    // 设置不自动换行
    m_table->setWordWrap(false);
    // 开启表格的排序功能，用户可通过点击表头对列数据排序
    m_table->setSortingEnabled(true);
    // 设置表头最后一列不自动拉伸填充剩余空间
    m_table->header()->setStretchLastSection(false);
    // 设置第 0 列的调整模式：根据内容自动调整宽度
    m_table->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // 设置第 1 列的调整模式：固定宽度（QHeaderView::Fixed 是枚举值，前面 QHeaderView:: 可简化，原写法冗余了 ）
    m_table->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    // 设置第 2 列的调整模式：固定宽度 
    m_table->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    // 手动设置第 1 列的宽度为 editorWidth（前面定义的 180）
    m_table->header()->resizeSection(1, editorWidth);
    // 手动设置第 2 列的宽度为 editorWidth
    m_table->header()->resizeSection(2, editorWidth);
    // 设置表格默认排序：按 ActionsModel 的 COLUMN_ACTION 列升序排列
    m_table->sortByColumn(ActionsModel::COLUMN_ACTION, Qt::AscendingOrder);
    // 给表格安装事件过滤器，处理按键事件（比如回车关闭窗口等，KeyPressFilter 里实现）
    m_table->installEventFilter(new KeyPressFilter(this));

    // 连接表格选择模型的 selectionChanged 信号：选中项变化时，在状态栏显示提示文本
    connect(m_table->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [&](const QItemSelection &selected, const QItemSelection &deselected) {
                // 调用 m_status 的 showText 方法，显示“点击选中的快捷键以显示编辑器”提示，持续 5 秒，用 AlternateBase 颜色
                m_status->showText(tr("Click on the selected shortcut to show the editor"),
                                   5,
                                   nullptr,
                                   QPalette::AlternateBase);
            });

    // 连接表格的 editRejected 信号（PrivateTreeView 自定义信号）：编辑被拒绝时，状态栏提示
    connect(m_table, &PrivateTreeView::editRejected, this, [&]() {
        // 显示“保留的快捷键无法编辑”提示，持续 5 秒，用 AlternateBase 颜色
        m_status->showText(tr("Reserved shortcuts can not be edited"),
                           5,
                           nullptr,
                           QPalette::AlternateBase);
    });

    // 连接选择模型的 currentChanged 信号：当前选中项变化时，处理列切换逻辑
    connect(m_table->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            [&](const QModelIndex &current) {
                // 如果当前选中的是第 0 列，就把当前选中项切换到同行动作的第 1 列（让快捷键编辑列获得焦点）
                if (current.column() == 0) {
                    m_table->setCurrentIndex(m_proxyModel->index(current.row(), 1));
                }
            });
    // 将表格添加到主垂直布局
    vlayout->addWidget(m_table);

    // 创建水平布局，用于放状态栏和按钮盒
    QHBoxLayout *hlayout = new QHBoxLayout();
    // 初始化状态标签部件（自定义的 StatusLabelWidget）
    m_status = new StatusLabelWidget();
    // 连接模型的 editError 信号：编辑出错时，在状态栏显示错误消息
    connect(&m_model, &ActionsModel::editError, this, [&](const QString &message) {
        m_status->showText(message, 5, nullptr);
    });
    // 将状态栏添加到水平布局
    hlayout->addWidget(m_status);

    // 按钮盒：仅创建“关闭”按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    // 设置“关闭”按钮不自动成为默认按钮（避免按回车误触）
    buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
    // 连接按钮盒的 rejected 信号到对话框的 reject 槽（关闭对话框）
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    // 将按钮盒添加到水平布局
    hlayout->addWidget(buttonBox);

    // 将水平布局添加到主垂直布局
    vlayout->addLayout(hlayout);
    // 设置对话框的主布局为 vlayout
    setLayout(vlayout);

    // 连接表格的 activated 信号：双击或回车激活项时，触发对应动作
    connect(m_table, &QAbstractItemView::activated, this, [&](const QModelIndex &index) {
        // 将代理模型的索引转换为源模型索引，获取对应的 QAction
        auto action = m_model.action(m_proxyModel->mapToSource(index));
        // 如果动作存在且可用，触发该动作
        if (action && action->isEnabled()) {
            action->trigger();
        }
    });

    // 计算表格初始宽度：基础 38 + 各列宽度之和
    int tableWidth = 38;
    for (int i = 0; i < m_table->model()->columnCount(); i++) {
        tableWidth += m_table->columnWidth(i);
    }
    // 设置对话框初始大小：宽度为计算的表格宽度，高度 600
    resize(tableWidth, 600);
}

// 保存当前正在编辑的快捷键内容
void ActionsDialog::saveCurrentEditor()
{
    // 获取当前编辑列的委托（ShortcutItemDelegate 类型，用于处理快捷键编辑）
    auto delegate = static_cast<ShortcutItemDelegate *>(
        m_table->itemDelegateForColumn(m_table->currentIndex().column()));
    if (delegate) { // 如果委托存在
        // 获取委托中当前使用的编辑器
        auto editor = delegate->currentEditor();
        if (editor && editor->seqEdit) { // 如果编辑器和其 seqEdit 部件有效
            // 将编辑器中的快捷键序列设置回代理模型对应索引
            m_proxyModel->setData(m_table->currentIndex(), editor->seqEdit->keySequence());
            // 发出关闭编辑器的信号（通知委托销毁编辑器）
            emit delegate->closeEditor(editor);
        }
    }
}

// 聚焦搜索结果：把表格焦点设为代理模型第 0 行、第 1 列的项
void ActionsDialog::focusSearchResults()
{
    m_table->setCurrentIndex(m_proxyModel->index(0, 1));
    // 让表格获取焦点，方便键盘操作
    m_table->setFocus();
}

// 对话框隐藏事件：隐藏时保存当前正在编辑的快捷键
void ActionsDialog::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event) // 标记 event 参数未使用
    saveCurrentEditor();
}

// 对话框显示事件：显示时让搜索框获取焦点、清空搜索内容、清除表格选中状态
void ActionsDialog::showEvent(QShowEvent *event)
{
    Q_UNUSED(event) // 标记 event 参数未使用
    m_searchField->setFocus(); // 搜索框获取焦点
    m_searchField->clear(); // 清空搜索框文本
    m_table->clearSelection(); // 清除表格选中项
}
