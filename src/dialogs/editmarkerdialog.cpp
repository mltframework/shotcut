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

// 包含当前类的头文件
#include "editmarkerdialog.h"

// 引入依赖的头文件（日志、自定义控件等）
#include "Logger.h"                  // 日志工具类，用于打印调试信息
#include "widgets/editmarkerwidget.h"// 自定义的标记编辑控件（封装标记的文本、颜色、时间范围等编辑功能）

// 引入Qt基础组件头文件
#include <QDebug>
#include <QDialogButtonBox>          // 对话框按钮组（包含确定、取消等按钮）
#include <QVBoxLayout>               // 垂直布局管理器（用于排列控件）


// 【构造函数】：初始化标记编辑对话框 
// 参数说明：
// - parent：父窗口指针
// - text：标记的初始文本
// - color：标记的初始颜色
// - start：标记的起始帧位置
// - end：标记的结束帧位置
// - maxEnd：标记结束帧的最大值（限制结束帧不能超过此值）
EditMarkerDialog::EditMarkerDialog(
    QWidget *parent, const QString &text, const QColor &color, int start, int end, int maxEnd)
    : QDialog(parent)  // 初始化父类QDialog，指定父窗口
{
    setWindowTitle(tr("Edit Marker"));  // 设置对话框标题（"编辑标记"）

    // 1. 创建垂直布局（管理对话框内的控件排列）
    QVBoxLayout *VLayout = new QVBoxLayout(this);

    // 2. 创建自定义标记编辑控件（EditMarkerWidget），并传入初始参数
    m_sWidget = new EditMarkerWidget(this, text, color, start, end, maxEnd);
    VLayout->addWidget(m_sWidget);  // 将自定义控件添加到垂直布局

    // 3. 创建按钮组（包含"确定"和"取消"按钮）
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    VLayout->addWidget(m_buttonBox);  // 将按钮组添加到垂直布局

    // 4. 连接按钮点击信号到自定义槽函数（处理确定/取消逻辑）
    connect(m_buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(clicked(QAbstractButton *)));

    // 5. 设置对话框布局及属性
    setLayout(VLayout);                // 为对话框设置布局
    setModal(true);                    // 模态对话框（阻塞父窗口操作）
    layout()->setSizeConstraint(QLayout::SetFixedSize);  // 固定对话框大小（不可拉伸）
}

// 【公共方法】：获取标记的文本 
QString EditMarkerDialog::getText()
{
    // 调用自定义控件的getText()方法，获取用户编辑后的标记文本
    return m_sWidget->getText();
}

//【公共方法】：获取标记的颜色 
QColor EditMarkerDialog::getColor()
{
    // 调用自定义控件的getColor()方法，获取用户选择的标记颜色
    return m_sWidget->getColor();
}

// 【公共方法】：获取标记的起始帧位置
int EditMarkerDialog::getStart()
{
    // 调用自定义控件的getStart()方法，获取用户设置的标记起始帧
    return m_sWidget->getStart();
}

// 【 公共方法】：获取标记的结束帧位置 
int EditMarkerDialog::getEnd()
{
    // 调用自定义控件的getEnd()方法，获取用户设置的标记结束帧
    return m_sWidget->getEnd();
}

//【槽函数】：处理按钮点击事件 
void EditMarkerDialog::clicked(QAbstractButton *button)
{
    // 获取点击按钮的角色（确定/取消/其他）
    QDialogButtonBox::ButtonRole role = m_buttonBox->buttonRole(button);
    
    if (role == QDialogButtonBox::AcceptRole) {
        // 确定按钮（AcceptRole）：关闭对话框并返回"成功"状态
        accept();
    } else if (role == QDialogButtonBox::RejectRole) {
        // 取消按钮（RejectRole）：关闭对话框并返回"取消"状态
        reject();
    } else {
        // 其他未知角色：打印调试日志
        LOG_DEBUG() << "Unknown role" << role;
    }
}
