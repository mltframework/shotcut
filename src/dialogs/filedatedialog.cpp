/*
 * Copyright (c) 2019-2022 Meltytech, LLC
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
#include "filedatedialog.h"

// 引入依赖的头文件（日志、多媒体控制、代理管理等）
#include "Logger.h"                  // 日志工具类，用于打印调试信息
#include "mltcontroller.h"           // MLT多媒体框架控制器（获取视频配置）
#include "proxymanager.h"            // 代理管理器（获取文件资源路径）
#include "shotcut_mlt_properties.h"  // Shotcut自定义MLT属性常量

// 引入MLT和Qt相关头文件
#include <MltProducer.h>             // MLT生产者类（处理音视频文件）
#include <QComboBox>                 // 下拉选择框（用于选择日期选项）
#include <QDateTimeEdit>             // 日期时间编辑框（用于手动修改日期）
#include <QDebug>
#include <QDialogButtonBox>          // 对话框按钮组（包含确定、取消按钮）
#include <QFileInfo>                 // 文件信息类（获取文件创建/修改时间）
#include <QVBoxLayout>               // 垂直布局管理器（排列界面控件）


// 【辅助函数】：向下拉框添加日期选项
// 参数说明：
// - combo：目标下拉框对象
// - description：日期选项的描述文本（如“当前值”“系统修改时间”）
// - date：日期时间数据（QDateTime类型）
void addDateToCombo(QComboBox *combo, const QString &description, const QDateTime &date)
{
    QDateTime local = date.toLocalTime();  // 将日期转换为本地时间
    // 拼接选项文本：本地时间字符串 + 描述（如“2024-01-01 12:00:00 [当前值]”）
    QString text = local.toString("yyyy-MM-dd HH:mm:ss") + " [" + description + "]";
    // 向下拉框添加选项（文本显示拼接内容，数据存储本地时间）
    combo->addItem(text, local);
}


// 【构造函数】：初始化文件日期设置对话框
// 参数说明：
// - title：对话框标题前缀（如“视频”“音频”，最终标题为“XXX File Date”）
// - producer：MLT生产者对象（关联待设置日期的音视频文件）
// - parent：父窗口指针
FileDateDialog::FileDateDialog(QString title, Mlt::Producer *producer, QWidget *parent)
    : QDialog(parent)
    , m_producer(producer)                // 保存MLT生产者对象（用于后续设置日期）
    , m_dtCombo(new QComboBox())          // 初始化日期选择下拉框
    , m_dtEdit(new QDateTimeEdit())       // 初始化日期时间编辑框
{
    // 设置对话框标题（拼接前缀和固定文本，如“视频 File Date”）
    setWindowTitle(tr("%1 File Date").arg(title));

    // 1. 获取生产者当前的创建时间（毫秒级时间戳）
    int64_t milliseconds = producer->get_creation_time();
    QDateTime creation_time;
    if (!milliseconds) {
        // 若未获取到时间戳，默认设为当前系统时间
        creation_time = QDateTime::currentDateTime();
    } else {
        // 若获取到时间戳，转换为QDateTime类型
        creation_time = QDateTime::fromMSecsSinceEpoch(milliseconds);
    }

    // 2. 创建垂直布局（管理对话框内控件排列）
    QVBoxLayout *VLayout = new QVBoxLayout(this);

    // 3. 填充日期选项到下拉框（从生产者、文件系统、元数据中获取可选日期）
    populateDateOptions(producer);
    m_dtCombo->setCurrentIndex(-1);  // 初始不选中任何选项
    VLayout->addWidget(m_dtCombo);   // 将下拉框添加到布局
    // 连接下拉框选择变化信号到槽函数（选择不同日期时更新编辑框）
    connect(m_dtCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(dateSelected(int)));

    // 4. 配置日期时间编辑框
    m_dtEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");  // 设置显示格式（年-月-日 时:分:秒）
    m_dtEdit->setCalendarPopup(true);                   // 启用日历弹窗（方便选择日期）
    m_dtEdit->setTimeSpec(Qt::LocalTime);               // 设置时间标准为本地时间
    m_dtEdit->setDateTime(creation_time);               // 设置初始日期（生产者当前时间）
    VLayout->addWidget(m_dtEdit);  // 将编辑框添加到布局

    // 5. 创建按钮组（包含“确定”和“取消”按钮）
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
    VLayout->addWidget(buttonBox);  // 将按钮组添加到布局
    // 连接按钮信号到对话框默认槽函数（确定→接受，取消→拒绝）
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // 6. 设置对话框布局和属性
    this->setLayout(VLayout);  // 为对话框设置布局
    this->setModal(true);      // 模态对话框（阻塞父窗口操作）
}


// 【槽函数】：处理“确定”按钮点击（保存设置的日期到生产者）
void FileDateDialog::accept()
{
    // 将编辑框中的日期转换为本地时间的毫秒级时间戳，设置到生产者的创建时间属性
    m_producer->set_creation_time(
        (int64_t) m_dtEdit->dateTime().toTimeSpec(Qt::LocalTime).toMSecsSinceEpoch());
    QDialog::accept();  // 调用父类accept()，关闭对话框并返回成功状态
}


// 【槽函数】：处理下拉框选择变化（更新编辑框的日期）
// 参数：下拉框选中项的索引（-1表示未选中）
void FileDateDialog::dateSelected(int index)
{
    LOG_DEBUG() << index;  // 打印选中项索引（调试用）
    if (index > -1) {
        // 若选中有效项，从下拉框中获取对应日期数据，设置到编辑框
        m_dtEdit->setDateTime(m_dtCombo->itemData(index).toDateTime());
    }
}


// 【私有方法】：填充日期选项到下拉框（从多渠道获取可选日期）
// 参数：待获取日期的MLT生产者对象
void FileDateDialog::populateDateOptions(Mlt::Producer *producer)
{
    QDateTime dateTime;

    // 1. 添加生产者当前的创建时间（若存在）
    int64_t milliseconds = producer->get_creation_time();
    if (milliseconds) {
        dateTime = QDateTime::fromMSecsSinceEpoch(milliseconds);
        addDateToCombo(m_dtCombo, tr("Current Value"), dateTime);  // 描述为“当前值”
    }

    // 2. 添加当前系统时间
    addDateToCombo(m_dtCombo, tr("Now"), QDateTime::currentDateTime());  // 描述为“Now”（现在）

    // 3. 添加文件系统中的日期（修改时间、创建时间）
    QString resource = ProxyManager::resource(*producer);  // 获取生产者关联的文件路径
    QFileInfo fileInfo(resource);                          // 创建文件信息对象
    if (fileInfo.exists()) {  // 若文件存在
        // 添加文件最后修改时间（描述为“System - Modified”）
        addDateToCombo(m_dtCombo, tr("System - Modified"), fileInfo.lastModified());
        // 添加文件创建时间（描述为“System - Created”）
        addDateToCombo(m_dtCombo, tr("System - Created"), fileInfo.birthTime());
    }

    // 4. 添加文件元数据中的日期（FFmpeg标准创建时间、QuickTime创建时间）
    // 创建临时生产者（用于读取元数据，避免影响原生产者）
    Mlt::Producer tmpProducer(MLT.profile(), "avformat", resource.toUtf8().constData());
    if (tmpProducer.is_valid()) {  // 若临时生产者有效
        // 4.1 读取FFmpeg标准创建时间（元数据键：meta.attr.creation_time.markup）
        dateTime = QDateTime::fromString(tmpProducer.get("meta.attr.creation_time.markup"),
                                         Qt::ISODateWithMs);
        if (dateTime.isValid()) {  // 若日期有效，添加到下拉框（描述为“Metadata - Creation Time”）
            addDateToCombo(m_dtCombo, tr("Metadata - Creation Time"), dateTime);
        }

        // 4.2 读取QuickTime格式的创建时间（元数据键：meta.attr.com.apple.quicktime.creationdate.markup）
        dateTime = QDateTime::fromString(tmpProducer.get(
                                             "meta.attr.com.apple.quicktime.creationdate.markup"),
                                         Qt::ISODateWithMs);
        if (dateTime.isValid()) {  // 若日期有效，添加到下拉框（描述为“Metadata - QuickTime date”）
            addDateToCombo(m_dtCombo, tr("Metadata - QuickTime date"), dateTime);
        }
    }
}
