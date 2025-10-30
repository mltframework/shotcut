/*
 * Copyright (c) 2020 Meltytech, LLC
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
#include "slideshowgeneratordialog.h"

// 引入依赖的头文件（日志、自定义控件等）
#include "Logger.h"                          // 日志工具类，用于打印调试信息
#include "widgets/slideshowgeneratorwidget.h"// 幻灯片生成自定义控件（封装幻灯片配置与生成逻辑）

// 引入MLT和Qt相关头文件
#include <MltProfile.h>                      // MLT配置类（用于幻灯片的视频参数配置）
#include <MltTransition.h>                   // MLT转场类（幻灯片中图片切换的过渡效果）
#include <QDebug>
#include <QDialogButtonBox>                  // 对话框按钮组（包含确定、关闭按钮）
#include <QVBoxLayout>                       // 垂直布局管理器（排列界面控件）


// 【构造函数】：初始化幻灯片生成对话框
// 参数说明：
// - parent：父窗口指针
// - clips：MLT播放列表对象（包含待生成幻灯片的图片/视频片段）
SlideshowGeneratorDialog::SlideshowGeneratorDialog(QWidget *parent, Mlt::Playlist &clips)
    : QDialog(parent)  // 初始化父类QDialog，指定父窗口
{
    // 设置对话框标题：包含“幻灯片生成器”和片段数量（如“Slideshow Generator - 5 Clips”）
    setWindowTitle(tr("Slideshow Generator - %n Clips", nullptr, clips.count()));

    // 1. 创建垂直布局（管理对话框内的控件排列）
    QVBoxLayout *VLayout = new QVBoxLayout(this);

    // 2. 创建幻灯片生成自定义控件，传入待处理的播放列表和父窗口
    m_sWidget = new SlideshowGeneratorWidget(&clips, this);
    VLayout->addWidget(m_sWidget);  // 将自定义控件添加到垂直布局

    // 3. 创建按钮组（包含“确定”和“关闭”按钮）
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Close);
    VLayout->addWidget(m_buttonBox);  // 将按钮组添加到垂直布局
    // 连接按钮点击信号到自定义槽函数（处理确定/关闭逻辑）
    connect(m_buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(clicked(QAbstractButton *)));

    // 4. 设置对话框布局及属性
    setLayout(VLayout);                // 为对话框设置布局
    setModal(true);                    // 模态对话框（阻塞父窗口操作）
    layout()->setSizeConstraint(QLayout::SetFixedSize);  // 固定对话框大小（不可拉伸）
}


// 【公共方法】：获取生成的幻灯片播放列表
// 返回值：Mlt::Playlist* - 包含幻灯片所有片段（图片+转场）的MLT播放列表，供外部使用（如导出、预览）
Mlt::Playlist *SlideshowGeneratorDialog::getSlideshow()
{
    // 调用自定义控件的getSlideshow()方法，获取生成的幻灯片播放列表
    return m_sWidget->getSlideshow();
}


// 【私有槽函数】：处理按钮点击事件（响应确定、关闭等按钮操作）
// 参数：button - 被点击的按钮对象（QAbstractButton类型）
void SlideshowGeneratorDialog::clicked(QAbstractButton *button)
{
    // 获取点击按钮的角色（确定→AcceptRole，关闭→RejectRole，其他→未知角色）
    QDialogButtonBox::ButtonRole role = m_buttonBox->buttonRole(button);
    
    if (role == QDialogButtonBox::AcceptRole) {
        // 确定按钮：打印调试日志，关闭对话框并返回“成功”状态
        LOG_DEBUG() << "Accept";
        accept();
    } else if (role == QDialogButtonBox::RejectRole) {
        // 关闭按钮：打印调试日志，关闭对话框并返回“取消”状态
        LOG_DEBUG() << "Reject";
        reject();
    } else {
        // 其他未知角色：打印调试日志（便于排查异常点击）
        LOG_DEBUG() << "Unknown role" << role;
    }
}
