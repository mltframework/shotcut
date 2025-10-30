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

// 【防止头文件重复包含的保护宏】：如果未定义ALIGNAUDIODIALOG_H，则进入定义
#ifndef ALIGNAUDIODIALOG_H
#define ALIGNAUDIODIALOG_H

// 包含对齐剪辑数据模型的头文件（提供AlignClipsModel类，用于管理剪辑对齐相关数据）
#include "models/alignclipsmodel.h"

// 包含Qt基础对话框类头文件（AlignAudioDialog继承自QDialog）
#include <QDialog>
// 包含QUuid类头文件（用于标识待对齐的剪辑，确保唯一性）
#include <QUuid>

// 【前向声明】：避免包含不必要的头文件，减少编译依赖，提升编译效率
class QComboBox;          // 下拉选择框控件（用于选择参考轨道、速度调整范围）
class QDialogButtonBox;   // 对话框按钮组控件（整合确定、取消、应用等按钮）
class QLabel;             // 文本标签控件（用于显示提示文本）
class QListWidget;        // 列表控件（此处未实际使用，可能为预留或历史兼容）
class QLineEdit;          // 单行文本输入框（此处未实际使用，可能为预留或历史兼容）
class QPushButton;        // 按钮控件（用于自定义功能按钮，如“处理”“应用”）
class QTreeView;          // 树状视图控件（用于显示待对齐剪辑的列表及状态）

class AlignTableDelegate; // 前向声明自定义表格委托类（用于自定义QTreeView单元格绘制，如进度条、状态图标）
class MultitrackModel;    // 前向声明多轨道数据模型类（管理时间线中的轨道和剪辑数据）
class LongUiTask;         // 前向声明长时间UI任务类（用于显示音频对齐过程中的进度窗口）

// 定义音频对齐对话框类，继承自Qt标准对话框QDialog
class AlignAudioDialog : public QDialog
{
    Q_OBJECT  // Qt元对象系统宏：启用信号槽机制、运行时类型信息等Qt核心功能

public:
    // 构造函数：初始化音频对齐对话框
    // 参数说明：
    // - title：对话框窗口标题
    // - model：多轨道数据模型指针（用于获取轨道和剪辑信息）
    // - uuids：待对齐剪辑的唯一标识列表（QUuid数组，指定需要处理的剪辑）
    // - parent：父窗口指针（用于Qt对象树管理，默认值为0表示无父窗口）
    explicit AlignAudioDialog(QString title,
                              MultitrackModel *model,
                              const QVector<QUuid> &uuids,
                              QWidget *parent = 0);
    // 虚析构函数：确保子类析构时能正确调用父类析构，释放资源（此处虽无子类，但符合Qt类设计规范）
    virtual ~AlignAudioDialog();

private slots:
    // 私有槽函数：重建待对齐剪辑列表（当参考轨道或速度调整范围变化时调用）
    void rebuildClipList();
    // 私有槽函数：执行音频对齐处理（分析参考轨道和剪辑音频，计算对齐参数）
    void process();
    // 私有槽函数：应用对齐结果（将计算出的偏移量、速度等参数应用到时间线剪辑）
    void apply();
    // 私有槽函数：执行处理并直接应用结果（合并process()和apply()步骤，一步完成对齐）
    void processAndApply();
    // 私有槽函数：更新参考轨道的分析进度（接收进度信号，更新进度显示）
    // 参数percent：当前进度百分比（0-100）
    void updateReferenceProgress(int percent);
    // 私有槽函数：更新单个剪辑的分析进度（接收进度信号，更新列表中对应剪辑的进度条）
    // 参数index：剪辑在列表中的索引；percent：当前进度百分比（0-100）
    void updateClipProgress(int index, int percent);
    // 私有槽函数：单个剪辑对齐分析完成（接收完成信号，更新剪辑的对齐参数和状态）
    // 参数index：剪辑索引；offset：时间偏移量；speed：速度补偿值；quality：对齐质量（0-1）
    void clipFinished(int index, int offset, double speed, double quality);

private:
    // 私有成员变量：存储对话框的核心组件和数据
    AlignTableDelegate *m_delegate;          // 自定义表格委托指针（用于QTreeView的单元格自定义绘制）
    MultitrackModel *m_model;                // 多轨道数据模型指针（指向外部传入的时间线轨道数据）
    AlignClipsModel m_alignClipsModel;       // 对齐剪辑数据模型对象（管理待对齐剪辑的名称、进度、对齐参数等）
    QVector<QUuid> m_uuids;                  // 待对齐剪辑的唯一标识列表（存储需要处理的剪辑UUID）
    QComboBox *m_trackCombo;                 // 参考轨道下拉框指针（用于选择哪个轨道作为对齐基准）
    QComboBox *m_speedCombo;                 // 速度调整范围下拉框指针（用于选择对齐时允许的速度补偿范围）
    QTreeView *m_table;                      // 剪辑列表视图指针（显示待对齐剪辑的信息，如名称、进度、偏移量）
    QDialogButtonBox *m_buttonBox;           // 对话框按钮组指针（整合取消、处理、应用等按钮）
    QPushButton *m_applyButton;              // “应用”按钮指针（单独触发应用对齐结果的操作）
    QPushButton *m_processAndApplyButton;    // “处理并应用”按钮指针（触发“分析+应用”的合并操作）
    LongUiTask *m_uiTask;                    // 长时间任务指针（显示音频分析过程中的进度窗口，提升用户体验）
};

// 结束头文件保护宏：如果已定义ALIGNAUDIODIALOG_H，则跳过上述内容
#endif // ALIGNAUDIODIALOG_H
