/*
 * Copyright (c) 2021 Meltytech, LLC
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

// 头文件保护宏，防止重复包含
#ifndef MULTIFILEEXPORTDIALOG_H
#define MULTIFILEEXPORTDIALOG_H

// 引入Qt基础类
#include <QDialog>
#include <QStringList>

// 前向声明，减少编译依赖
class QComboBox;          // 下拉选择框，用于选择文件名字段类型
class QDialogButtonBox;   // 对话框按钮组，包含确定和取消按钮
class QLabel;             // 标签控件，用于显示文本提示或错误信息
class QListWidget;        // 列表控件，用于展示导出文件列表
class QLineEdit;          // 单行输入框，用于输入路径、前缀等文本
namespace Mlt {
class Playlist;           // MLT播放列表类，存储待导出的媒体片段
}

// 多文件导出对话框类，用于配置并生成多个媒体文件的导出路径
class MultiFileExportDialog : public QDialog
{
    Q_OBJECT  // 启用Qt元对象系统，支持信号和槽

public:
    // 【构造函数】：初始化多文件导出对话框
    // 参数说明：
    // - title：对话框标题
    // - playlist：待导出的MLT播放列表
    // - directory：默认导出目录
    // - prefix：文件名前缀
    // - extension：文件扩展名
    // - parent：父窗口指针
    explicit MultiFileExportDialog(QString title,
                                   Mlt::Playlist *playlist,
                                   const QString &directory,
                                   const QString &prefix,
                                   const QString &extension,
                                   QWidget *parent = 0);

    // 【公共方法】：获取所有导出文件的路径列表
    // 返回值：包含所有导出文件完整路径的字符串列表
    QStringList getExportFiles();

private slots:
    // 【私有槽函数】：重新生成导出文件列表
    // 触发时机：当配置参数（如前缀、字段、扩展名）改变时
    void rebuildList();

    // 【私有槽函数】：打开目录浏览对话框
    // 功能：允许用户选择导出文件的保存目录
    void browse();

private:
    // 【私有方法】：拼接文件名字段
    // 参数：
    // - text：当前文件名文本
    // - combo：选择字段类型的下拉框
    // - clipIndex：片段在播放列表中的索引
    // 返回值：拼接字段后的文件名文本
    QString appendField(QString text, QComboBox *combo, int clipIndex);

    // 【私有方法】：向下拉框填充文件名字段选项
    // 参数：combo - 目标下拉框
    void fillCombo(QComboBox *combo);

    // 【私有成员变量】：MLT播放列表指针
    // 作用：存储待导出的媒体片段集合
    Mlt::Playlist *m_playlist;

    // 【私有成员变量】：目录输入框指针
    // 作用：显示当前选择的导出目录
    QLineEdit *m_dir;

    // 【私有成员变量】：前缀输入框指针
    // 作用：输入文件名的前缀部分
    QLineEdit *m_prefix;

    // 【私有成员变量】：字段选择下拉框（3个）
    // 作用：选择用于生成文件名的动态字段（如名称、索引等）
    QComboBox *m_field1;
    QComboBox *m_field2;
    QComboBox *m_field3;

    // 【私有成员变量】：扩展名输入框指针
    // 作用：输入导出文件的扩展名
    QLineEdit *m_ext;

    // 【私有成员变量】：错误提示相关控件
    // 作用：显示错误图标和错误信息文本
    QLabel *m_errorIcon;
    QLabel *m_errorText;

    // 【私有成员变量】：文件列表控件指针
    // 作用：展示所有生成的导出文件路径
    QListWidget *m_list;

    // 【私有成员变量】：按钮组指针
    // 作用：包含确定和取消按钮，控制对话框关闭逻辑
    QDialogButtonBox *m_buttonBox;

    // 【私有成员变量】：导出文件路径列表
    // 作用：存储所有生成的导出文件完整路径
    QStringList m_stringList;
};

// 结束头文件保护宏
#endif // MULTIFILEEXPORTDIALOG_H
