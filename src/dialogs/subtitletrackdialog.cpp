/*
 * Copyright (c) 2024 Meltytech, LLC
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
#include "subtitletrackdialog.h"

// 引入Qt相关头文件
#include <QComboBox>                 // 下拉选择框（用于选择字幕语言）
#include <QDialogButtonBox>          // 对话框按钮组（包含确定、取消按钮）
#include <QGridLayout>               // 网格布局管理器（排列界面控件）
#include <QLabel>                    // 标签控件（显示文本提示）
#include <QLineEdit>                 // 单行输入框（输入字幕轨道名称）
#include <QList>                     // 列表容器（存储所有Locale对象）
#include <QLocale>                   // 本地化类（获取语言列表、语言编码等）


// 【辅助函数】：向下拉框填充所有支持的语言选项（含语言名称和ISO639-2编码）
// 参数：combo - 目标下拉框（用于展示语言列表）
static void fillLanguages(QComboBox *combo)
{
    // 1. 获取所有可用的Locale（匹配任意语言、任意文字、任意地区）
    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage,
                                                         QLocale::AnyScript,
                                                         QLocale::AnyTerritory);

    // 2. 构建“语言名称→ISO639-2编码”的映射（去重，确保每种语言只出现一次）
    QMap<QString, QString> iso639_2LanguageCodes;
    for (const QLocale &locale : allLocales) {
        QLocale::Language lang = locale.language();
        // 过滤无效语言（排除“任意语言”和“C语言”）
        if (lang != QLocale::AnyLanguage && lang != QLocale::C) {
            // 获取语言的ISO639-2编码（如“eng”代表英语，“chi”代表中文）
            QString langCode = QLocale::languageToCode(lang, QLocale::ISO639Part2);
            // 获取语言的显示名称（如“English”“Chinese”）
            QString langStr = QLocale::languageToString(lang);
            // 确保编码和名称非空，避免无效数据
            if (!langCode.isEmpty() && !langStr.isEmpty()) {
                iso639_2LanguageCodes.insert(langStr, langCode);  // 插入映射（自动去重，键唯一）
            }
        }
    }

    // 3. 遍历映射，向下拉框添加语言选项（文本格式：“语言名称 (编码)”，数据存储编码）
    for (auto it = iso639_2LanguageCodes.keyValueBegin(); it != iso639_2LanguageCodes.keyValueEnd();
         ++it) {
        // 构建选项文本（如“English (eng)”“Chinese (chi)”）
        QString text = QStringLiteral("%1 (%2)").arg(it->first).arg(it->second);
        // 添加选项：文本显示完整信息，数据存储ISO639-2编码（后续用于标识语言）
        combo->addItem(text, it->second);
    }
}


// 【构造函数】：初始化新建字幕轨道对话框
// 参数说明：
// - name：字幕轨道默认名称（如“Subtitle 1”）
// - lang：字幕轨道默认语言编码（ISO639-2格式，如“eng”）
// - parent：父窗口指针
SubtitleTrackDialog::SubtitleTrackDialog(const QString &name, const QString &lang, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("New Subtitle Track"));  // 设置对话框标题（“新建字幕轨道”）

    // 1. 创建网格布局（管理对话框内所有控件的排列）
    QGridLayout *grid = new QGridLayout();

    // 2. 字幕轨道名称行（标签 + 输入框）
    // 添加“名称”标签（右对齐，与输入框对齐）
    grid->addWidget(new QLabel(tr("Name")), 0, 0, Qt::AlignRight);
    m_name = new QLineEdit(this);          // 创建名称输入框
    m_name->setText(name);                 // 设置默认名称
    grid->addWidget(m_name, 0, 1);         // 将输入框添加到网格第0行第1列

    // 3. 字幕语言选择行（标签 + 下拉框）
    // 添加“语言”标签（右对齐）
    grid->addWidget(new QLabel(tr("Language")), 1, 0, Qt::AlignRight);
    m_lang = new QComboBox(this);          // 创建语言下拉框
    fillLanguages(m_lang);                 // 调用辅助函数填充语言选项
    // 从默认语言编码匹配下拉框选项，自动选中
    for (int i = 0; i < m_lang->count(); i++) {
        if (m_lang->itemData(i).toString() == lang) {
            m_lang->setCurrentIndex(i);
            break;
        }
    }
    grid->addWidget(m_lang, 1, 1);         // 将下拉框添加到网格第1行第1列

    // 4. 按钮组（确定 + 取消按钮）
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
    // 将按钮组添加到网格第2行，跨0-1列（居中显示）
    grid->addWidget(buttonBox, 2, 0, 2, 2);
    // 连接按钮信号到对话框默认槽函数（确定→接受，取消→拒绝）
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // 5. 配置对话框属性
    setLayout(grid);          // 设置对话框布局
    this->setModal(true);     // 模态对话框（阻塞父窗口操作）
    m_name->setFocus();       // 初始焦点设置在名称输入框（方便用户直接修改名称）
}


// 【公共方法】：获取用户输入的字幕轨道名称
// 返回值：QString - 名称输入框中的文本
QString SubtitleTrackDialog::getName()
{
    return m_name->text();
}


// 【公共方法】：获取用户选择的字幕语言编码（ISO639-2格式）
// 返回值：QString - 语言下拉框选中项存储的编码（如“eng”“chi”）
QString SubtitleTrackDialog::getLanguage()
{
    return m_lang->currentData().toString();
}


// 【槽函数】：处理“确定”按钮点击（调用父类accept()，关闭对话框并返回成功状态）
void SubtitleTrackDialog::accept()
{
    QDialog::accept();
}
