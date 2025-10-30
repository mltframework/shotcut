/*
 * Copyright (c) 2021-2022 Meltytech, LLC
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

// 包含当前类的头文件
#include "multifileexportdialog.h"

// 引入依赖的业务逻辑头文件
#include "mainwindow.h"              // 主窗口类（获取播放列表、剪辑信息等）
#include "proxymanager.h"            // 代理管理器（获取文件资源路径）
#include "qmltypes/qmlapplication.h" // QML应用类（获取对话框模态属性）
#include "shotcut_mlt_properties.h"  // Shotcut自定义MLT属性常量
#include "util.h"                   // 工具类（获取文件对话框选项、计算哈希等）

// 引入MLT和Qt相关头文件
#include <MltPlaylist.h>             // MLT播放列表类（管理待导出的剪辑）
#include <QComboBox>                 // 下拉选择框（用于选择文件名字段类型）
#include <QDialogButtonBox>          // 对话框按钮组（包含确定、取消按钮）
#include <QDir>                      // 目录操作类（处理导出目录）
#include <QFileDialog>               // 文件对话框（用于选择导出目录）
#include <QGridLayout>               // 网格布局管理器（排列界面控件）
#include <QHBoxLayout>               // 水平布局管理器（排列目录输入框和浏览按钮）
#include <QLabel>                    // 标签控件（显示文本提示）
#include <QLineEdit>                 // 单行输入框（输入前缀、扩展名等）
#include <QListWidget>               // 列表控件（展示导出文件列表）
#include <QPushButton>               // 按钮控件（浏览目录按钮）


// 【枚举】：文件名字段类型（用于组合生成导出文件名）
enum {
    NAME_FIELD_NONE = 0,   // 无字段（不添加内容）
    NAME_FIELD_NAME,       // 名称字段（剪辑标题或文件名）
    NAME_FIELD_INDEX,      // 索引字段（剪辑在播放列表中的序号）
    NAME_FIELD_DATE,       // 日期字段（剪辑的创建日期）
    NAME_FIELD_HASH,       // 哈希字段（剪辑的唯一哈希值）
};


// 【构造函数】：初始化多文件导出对话框
// 参数说明：
// - title：对话框标题（如“Export Multiple Files”）
// - playlist：MLT播放列表对象（包含待导出的剪辑）
// - directory：默认导出目录路径
// - prefix：默认文件名前缀
// - extension：默认文件扩展名
// - parent：父窗口指针
MultiFileExportDialog::MultiFileExportDialog(QString title,
                                             Mlt::Playlist *playlist,
                                             const QString &directory,
                                             const QString &prefix,
                                             const QString &extension,
                                             QWidget *parent)
    : QDialog(parent)
    , m_playlist(playlist)  // 保存播放列表对象（用于获取剪辑信息）
{
    int col = 0;  // 网格布局的列索引（用于定位控件）
    setWindowTitle(title);  // 设置对话框标题
    setWindowModality(QmlApplication::dialogModality());  // 设置模态属性（与应用一致）

    // 1. 创建网格布局（管理对话框内所有控件的排列）
    QGridLayout *glayout = new QGridLayout();
    glayout->setHorizontalSpacing(4);  // 水平间距4像素
    glayout->setVerticalSpacing(2);    // 垂直间距2像素

    // 2. 添加“导出目录”相关控件（标签、输入框、浏览按钮）
    glayout->addWidget(new QLabel(tr("Directory")), col, 0, Qt::AlignRight);  // 标签（右对齐）
    QHBoxLayout *dirHbox = new QHBoxLayout();  // 水平布局（排列输入框和按钮）
    // 目录输入框：显示默认目录，设为只读（只能通过浏览按钮修改）
    m_dir = new QLineEdit(QDir::toNativeSeparators(directory));
    m_dir->setReadOnly(true);
    // 浏览按钮：设置图标（系统主题图标或本地图标）
    QPushButton *browseButton = new QPushButton(this);
    browseButton->setIcon(
        QIcon::fromTheme("document-open", QIcon(":/icons/oxygen/32x32/actions/document-open.png")));
    // 连接浏览按钮点击信号到槽函数（兼容Qt4/Qt5信号格式）
    if (!connect(browseButton, &QAbstractButton::clicked, this, &MultiFileExportDialog::browse))
        connect(browseButton, SIGNAL(clicked()), SLOT(browse()));
    // 将输入框和按钮添加到水平布局
    dirHbox->addWidget(m_dir);
    dirHbox->addWidget(browseButton);
    glayout->addLayout(dirHbox, col++, 1, Qt::AlignLeft);  // 水平布局添加到网格（左对齐）

    // 3. 添加“文件名前缀”相关控件（标签、输入框）
    glayout->addWidget(new QLabel(tr("Prefix")), col, 0, Qt::AlignRight);
    // 前缀输入框：默认值为“export”（若传入前缀为空）
    m_prefix = new QLineEdit(prefix.isEmpty() ? tr("export") : prefix);
    // 连接文本变化信号到“重建文件列表”槽函数（修改前缀时更新列表）
    if (!connect(m_prefix, &QLineEdit::textChanged, this, &MultiFileExportDialog::rebuildList))
        connect(m_prefix, SIGNAL(textChanged(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_prefix, col++, 1, Qt::AlignLeft);

    // 4. 添加“字段1”相关控件（标签、下拉框）
    glayout->addWidget(new QLabel(tr("Field 1")), col, 0, Qt::AlignRight);
    m_field1 = new QComboBox();
    fillCombo(m_field1);  // 向下拉框添加字段类型选项
    // 连接下拉框选择变化信号到“重建文件列表”槽函数
    if (!connect(m_field1, QOverload<int>::of(&QComboBox::activated), this, &MultiFileExportDialog::rebuildList))
        connect(m_field1, SIGNAL(activated(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_field1, col++, 1, Qt::AlignLeft);

    // 5. 添加“字段2”相关控件（标签、下拉框）
    glayout->addWidget(new QLabel(tr("Field 2")), col, 0, Qt::AlignRight);
    m_field2 = new QComboBox();
    fillCombo(m_field2);  // 填充字段类型选项
    // 连接选择变化信号到“重建文件列表”槽函数
    if (!connect(m_field2, QOverload<int>::of(&QComboBox::activated), this, &MultiFileExportDialog::rebuildList))
        connect(m_field2, SIGNAL(activated(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_field2, col++, 1, Qt::AlignLeft);

    // 6. 添加“字段3”相关控件（标签、下拉框）
    glayout->addWidget(new QLabel(tr("Field 3")), col, 0, Qt::AlignRight);
    m_field3 = new QComboBox();
    fillCombo(m_field3);  // 填充字段类型选项
    m_field3->setCurrentIndex(NAME_FIELD_INDEX);  // 默认选中“索引”字段
    // 连接选择变化信号到“重建文件列表”槽函数
    if (!connect(m_field3, QOverload<int>::of(&QComboBox::activated), this, &MultiFileExportDialog::rebuildList))
        connect(m_field3, SIGNAL(activated(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_field3, col++, 1, Qt::AlignLeft);

    // 7. 添加“文件扩展名”相关控件（标签、输入框）
    glayout->addWidget(new QLabel(tr("Extension")), col, 0, Qt::AlignRight);
    m_ext = new QLineEdit(extension);  // 扩展名输入框（默认值为传入的扩展名）
    // 连接文本变化信号到“重建文件列表”槽函数
    if (!connect(m_ext, &QLineEdit::textChanged, this, &MultiFileExportDialog::rebuildList))
        connect(m_ext, SIGNAL(textChanged(const QString &)), SLOT(rebuildList()));
    glayout->addWidget(m_ext, col++, 1, Qt::AlignLeft);

    // 8. 添加“错误提示”相关控件（图标、文本）
    m_errorIcon = new QLabel();
    QIcon icon = QIcon(":/icons/oxygen/32x32/status/task-reject.png");  // 错误图标
    m_errorIcon->setPixmap(icon.pixmap(QSize(24, 24)));  // 设置图标大小
    glayout->addWidget(m_errorIcon, col, 0, Qt::AlignRight);  // 错误图标（右对齐）
    m_errorText = new QLabel();  // 错误文本标签
    glayout->addWidget(m_errorText, col++, 1, Qt::AlignLeft);  // 错误文本（左对齐）

    // 9. 添加“导出文件列表”控件（列表控件）
    m_list = new QListWidget();
    m_list->setSelectionMode(QAbstractItemView::NoSelection);  // 禁用选择模式
    m_list->setIconSize(QSize(16, 16));  // 设置图标大小（显示文件状态图标）
    glayout->addWidget(m_list, col++, 0, 1, 2);  // 列表占2列（跨列布局）

    // 10. 添加“按钮组”控件（确定、取消按钮）
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    glayout->addWidget(m_buttonBox, col++, 0, 1, 2);  // 按钮组占2列
    // 连接按钮信号到对话框默认槽函数
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // 11. 设置布局列宽（确保目录输入框和浏览按钮显示完整）
    glayout->setColumnMinimumWidth(1,
                                   fontMetrics().horizontalAdvance(m_dir->text())
                                       + browseButton->width());

    // 12. 配置对话框属性
    this->setLayout(glayout);  // 设置对话框布局
    this->setModal(true);      // 模态对话框（阻塞父窗口操作）
    rebuildList();             // 初始化时重建导出文件列表
    resize(400, 300);          // 设置对话框初始大小
}


// 【公共方法】：获取导出文件路径列表
// 返回值：QStringList - 所有待导出文件的完整路径
QStringList MultiFileExportDialog::getExportFiles()
{
    return m_stringList;
}


// 【私有方法】：拼接文件名字段（根据字段类型添加内容到文件名）
// 参数说明：
// - text：当前已拼接的文件名文本
// - combo：字段类型下拉框（选中的字段类型）
// - clipIndex：剪辑在播放列表中的索引
// 返回值：QString - 拼接字段后的文件名文本
QString MultiFileExportDialog::appendField(QString text, QComboBox *combo, int clipIndex)
{
    QString field;  // 存储当前字段的内容
    // 根据下拉框选中的字段类型，生成对应内容
    switch (combo->currentData().toInt()) {
    default:
    case NAME_FIELD_NONE:  // 无字段：不添加内容
        break;
    case NAME_FIELD_NAME: {  // 名称字段：获取剪辑标题或文件名
        // 获取当前剪辑的信息（智能指针自动释放资源）
        QScopedPointer<Mlt::ClipInfo> info(MAIN.playlist()->clip_info(clipIndex));
        if (info && info->producer && info->producer->is_valid()) {
            // 优先获取剪辑的标题属性
            field = info->producer->get(kShotcutCaptionProperty);
            if (field.isEmpty()) {
                // 标题为空时，获取文件路径的基础名称
                field = ProxyManager::resource(*info->producer);
                field = QFileInfo(field).completeBaseName();
            }
            if (field == "<producer>") {
                // 若为生产者对象，获取其MLT服务名称
                field = QString::fromUtf8(info->producer->get("mlt_service"));
            }
        }
        break;
    }
    case NAME_FIELD_INDEX: {  // 索引字段：生成带前置零的序号
        // 计算序号的位数（与播放列表总剪辑数匹配，如10个剪辑则为2位）
        int digits = QString::number(m_playlist->count()).size();
        // 生成序号字符串（如第1个剪辑，3位 digits 则为“001”）
        field = QStringLiteral("%1").arg(clipIndex + 1, digits, 10, QChar('0'));
        break;
    }
    case NAME_FIELD_DATE: {  // 日期字段：获取剪辑的创建日期
        QScopedPointer<Mlt::ClipInfo> info(MAIN.playlist()->clip_info(clipIndex));
        if (info && info->producer && info->producer->is_valid()) {
            // 获取剪辑的创建时间（毫秒级时间戳）
            int64_t ms = info->producer->get_creation_time();
            if (ms) {
                // 转换为“年月日-时分秒”格式（如“20240520-143000”）
                field = QDateTime::fromMSecsSinceEpoch(ms).toString("yyyyMMdd-HHmmss");
            }
        }
        break;
    }
    case NAME_FIELD_HASH: {  // 哈希字段：获取剪辑的唯一哈希值
        QScopedPointer<Mlt::ClipInfo> info(MAIN.playlist()->clip_info(clipIndex));
        // 调用工具类方法计算生产者的哈希值
        field = Util::getHash(*info->producer);
        break;
    }
    }

    // 拼接字段到现有文本（空文本直接返回字段，非空则用“-”连接）
    if (text.isEmpty()) {
        return field;
    } else if (field.isEmpty()) {
        return text;
    } else {
        return text + "-" + field;
    }
}


// 【私有方法】：向下拉框填充字段类型选项
// 参数：combo - 目标下拉框对象
void MultiFileExportDialog::fillCombo(QComboBox *combo)
{
    // 添加选项：文本显示名称，数据存储字段类型枚举值
    combo->addItem(tr("None"), QVariant(NAME_FIELD_NONE));
    combo->addItem(tr("Name"), QVariant(NAME_FIELD_NAME));
    combo->addItem(tr("Index"), QVariant(NAME_FIELD_INDEX));
    combo->addItem(tr("Date"), QVariant(NAME_FIELD_DATE));
    combo->addItem(tr("Hash"), QVariant(NAME_FIELD_HASH));
}


// 【私有方法】：重建导出文件列表（根据当前配置生成文件名，检测错误）
void MultiFileExportDialog::rebuildList()
{
    m_stringList.clear();  // 清空文件路径列表
    m_list->clear();       // 清空列表控件

    // 1. 遍历播放列表中的每个剪辑，生成对应的导出文件路径
    for (int i = 0; i < m_playlist->count(); i++) {
        QString filename = m_prefix->text();  // 初始化为前缀
        // 依次拼接3个字段（字段1→字段2→字段3）
        filename = appendField(filename, m_field1, i);
        filename = appendField(filename, m_field2, i);
        filename = appendField(filename, m_field3, i);
        // 若文件名非空，拼接目录和扩展名，生成完整路径
        if (!filename.isEmpty()) {
            filename = m_dir->text() + QDir::separator() + filename + "." + m_ext->text();
            m_stringList << filename;  // 添加到文件路径列表
        }
    }
    m_list->addItems(m_stringList);  // 将文件路径添加到列表控件

    // 2. 检测导出配置错误（空文件名、目录不存在、文件已存在、文件名重复）
    m_errorText->setText("");  // 清空错误文本
    int n = m_stringList.size();
    if (n == 0) {
        // 错误1：无导出文件（文件名为空）
        m_errorText->setText(tr("Empty File Name"));
    } else if (!QDir(m_dir->text()).exists()) {
        // 错误2：导出目录不存在
        m_errorText->setText(tr("Directory does not exist: %1").arg(m_dir->text()));
    } else {
        // 错误3：文件已存在或文件名重复
        for (int i = 0; i < n; i++) {
            QString errorString;
            QFileInfo fileInfo(m_stringList[i]);
            // 检测文件是否已存在
            if (fileInfo.exists()) {
                errorString = tr("File Exists: %1").arg(m_stringList[i]);
            } else {
                // 检测文件名是否与其他文件重复
                for (int j = 0; j < n; j++) {
                    if (j != i && m_stringList[i] == m_stringList[j]) {
                        QString filename = QFileInfo(m_stringList[i]).fileName();
                        errorString = tr("Duplicate File Name: %1").arg(filename);
                        break;
                    }
                }
            }

            // 根据错误状态设置列表项图标和提示
            QListWidgetItem *item = m_list->item(i);
            if (errorString.isEmpty()) {
                // 无错误：显示“完成”图标
                item->setIcon(QIcon(":/icons/oxygen/32x32/status/task-complete.png"));
            } else {
                // 有错误：显示“拒绝”图标，设置工具提示
                item->setIcon(QIcon(":/icons/oxygen/32x32/status/task-reject.png"));
                item->setToolTip(errorString);
                m_errorText->setText(errorString);  // 显示第一个错误
            }
        }
    }

    // 3. 根据错误状态更新界面（显示/隐藏错误提示，启用/禁用确定按钮）
    if (m_errorText->text().isEmpty()) {
        m_errorText->setVisible(false);
        m_errorIcon->setVisible(false);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);  // 启用确定按钮
    } else {
        m_errorText->setVisible(true);
        m_errorIcon->setVisible(true);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);  // 禁用确定按钮
    }
    // 设置确定按钮提示（提示用户修复错误后导出）
    m_buttonBox->button(QDialogButtonBox::Ok)->setToolTip(tr("Fix file name errors before export."));
}


// 【私有槽函数】：处理“浏览”按钮点击事件（选择导出目录）
void MultiFileExportDialog::browse()
{
    // 打开目录选择对话框（默认路径为当前目录，使用工具类的文件对话框选项）
    QString directory = QDir::toNativeSeparators(
        QFileDialog::getExistingDirectory(this,
                                          tr("Export Directory"),
                                          m_dir->text(),
                                          Util::getFileDialogOptions()));
    // 若选择了目录，更新目录输入框并重建文件列表
    if (!directory.isEmpty()) {
        m_dir->setText(directory);
        rebuildList();
    }
}
