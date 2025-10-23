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

// 包含当前类的头文件（声明AlignAudioDialog类）
#include "alignaudiodialog.h"

// 引入依赖的头文件（日志、命令、UI组件、业务逻辑等）
#include "Logger.h"                  // 日志工具类，用于打印调试/错误信息
#include "commands/timelinecommands.h"// 时间线操作命令类（如对齐剪辑的命令）
#include "dialogs/alignmentarray.h"  // 音频对齐数据数组类（存储音频特征值）
#include "dialogs/longuitask.h"      // 长时间UI任务类（显示处理进度）
#include "mainwindow.h"              // 主窗口类（访问全局资源如撤销栈）
#include "mltcontroller.h"           // MLT多媒体框架控制器（处理音视频数据）
#include "models/multitrackmodel.h"  // 多轨道数据模型（管理时间线轨道和剪辑）
#include "proxymanager.h"            // 代理管理器（处理音视频资源路径）
#include "qmltypes/qmlapplication.h" // QML应用类（获取对话框模态属性）
#include "settings.h"                // 配置类（保存/读取用户设置如参考轨道）
#include "shotcut_mlt_properties.h"  // Shotcut自定义MLT属性常量（如剪辑标题）
#include "util.h"                    // 工具类（如获取文件名、字符串处理）

// 引入Qt基础组件头文件
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTreeView>


// 【内部辅助类】AudioReader 
// 【功能】读取单个音频资源的特征数据（如音量平均值），用于后续对齐分析
class AudioReader : public QObject
{
    Q_OBJECT  // 启用Qt信号槽机制
public:
    // 构造函数：初始化音频读取参数
    // producerXml：MLT生产者的XML描述（包含音频资源信息）
    // array：存储音频特征数据的数组（输出结果）
    // in/out：音频的入点/出点（仅处理指定时间段，-1表示全段）
    AudioReader(QString producerXml, AlignmentArray *array, int in = -1, int out = -1)
        : QObject()
        , m_producerXml(producerXml)  // 保存MLT生产者XML
        , m_array(array)              // 保存特征数据数组指针
        , m_in(in)                    // 保存音频入点
        , m_out(out)                  // 保存音频出点
    {}

    // 【初始化特征数据数组】：设置数组最大长度（避免越界）
    void init(int maxLength) { m_array->init(maxLength); }

    // 【核心方法】：处理音频，计算每帧的音量平均值并存入特征数组
    void process()
    {
        // 1. 创建MLT生产者（从XML描述中加载音频资源）
        // QScopedPointer：智能指针，自动释放生产者对象，避免内存泄漏
        QScopedPointer<Mlt::Producer> producer(
            new Mlt::Producer(MLT.profile(), "xml-string", m_producerXml.toUtf8().constData()));
        
        // 2. 如果指定了入点/出点，设置生产者的处理范围（仅处理片段）
        if (m_in >= 0) {
            producer->set_in_and_out(m_in, m_out);
        }

        // 3. 获取音频总帧数，初始化特征值数组（存储每帧的音量平均值）
        size_t frameCount = producer->get_playtime();  // 音频总帧数
        std::vector<double> values(frameCount);        // 存储每帧特征值
        int progress = 0;                              // 进度值（0-100）

        // 4. 遍历每帧音频，计算音量平均值
        for (size_t i = 0; i < frameCount; ++i) {
            // 音频参数配置：采样率48000Hz、单声道、16位整型格式
            int frequency = 48000;
            int channels = 1;
            mlt_audio_format format = mlt_audio_s16;

            // 获取当前帧的音频数据
            std::unique_ptr<Mlt::Frame> frame(producer->get_frame(i));  // 智能指针管理帧对象
            mlt_position position = mlt_frame_get_position(frame->get_frame());  // 获取帧位置
            // 计算当前帧的采样点数（根据帧率和采样率推导）
            int samples = mlt_audio_calculate_frame_samples(float(producer->get_fps()),
                                                            frequency,
                                                            position);
            // 提取音频原始数据（16位整型指针）
            int16_t *data = static_cast<int16_t *>(
                frame->get_audio(format, frequency, channels, samples));

            // 5. 计算当前帧的音量总和（取绝对值，避免正负抵消）
            double sampleTotal = 0;
            for (int k = 0; k < samples; ++k) {
                sampleTotal += std::abs(data[k]);  // 累加每个采样点的绝对值
            }

            // 6. 计算当前帧的音量平均值，存入特征数组
            values[i] = sampleTotal / samples;

            // 7. 更新进度（仅当进度变化时发送信号，减少UI刷新频率）
            int newProgress = 100 * i / frameCount;  // 计算当前进度（百分比）
            if (newProgress != progress) {
                progress = newProgress;
                emit progressUpdate(progress);  // 发送进度更新信号
            }
        }

        // 8. 将计算好的特征值数组存入外部传入的AlignmentArray
        m_array->setValues(values);
    }

signals:
    // 进度更新信号：参数为当前进度（0-100），用于UI显示
    void progressUpdate(int);

private:
    QString m_producerXml;    // MLT生产者的XML描述（音频资源信息）
    AlignmentArray *m_array;  // 存储音频特征数据的外部数组
    int m_in;                 // 音频处理入点（帧位置）
    int m_out;                // 音频处理出点（帧位置）
};


//  【内部辅助类】：ClipAudioReader 
// 【功能】：读取单个剪辑的音频特征，并与参考轨道的音频特征对比，计算对齐参数（偏移量、速度）
class ClipAudioReader : public QObject
{
    Q_OBJECT  // 启用Qt信号槽机制
public:
    // 构造函数：初始化剪辑音频读取和对齐分析参数
    // producerXml：剪辑的MLT生产者XML
    // referenceArray：参考轨道的音频特征数组（用于对比）
    // index：剪辑在列表中的索引（标识当前处理的剪辑）
    // in/out：剪辑的音频入点/出点
    ClipAudioReader(QString producerXml, AlignmentArray &referenceArray, int index, int in, int out)
        : QObject()
        , m_referenceArray(referenceArray)  // 参考轨道特征数组（引用，避免拷贝）
        , m_reader(producerXml, &m_clipArray, in, out)  // 初始化音频读取器（输出到内部数组）
        , m_index(index)  // 剪辑在列表中的索引
    {
        // 连接AudioReader的进度信号到当前类的槽函数（转发进度）
        connect(&m_reader, SIGNAL(progressUpdate(int)), this, SLOT(onReaderProgressUpdate(int)));
    }

    // 初始化内部特征数组：与参考轨道数组长度一致
    void init(int maxLength) { m_reader.init(maxLength); }

    // 启动音频处理（异步执行，使用QtConcurrent多线程）
    void start() { m_future = QtConcurrent::run(&ClipAudioReader::process, this); }

    // 检查处理是否完成（用于主线程等待）
    bool isFinished() { return m_future.isFinished(); }

    // 核心方法：处理剪辑音频并计算对齐参数
    void process()
    {
        // 1. 发送初始进度（0%）
        onReaderProgressUpdate(0);
        // 2. 调用AudioReader处理剪辑音频，生成特征数组（m_clipArray）
        m_reader.process();

        // 3. 计算对齐参数（偏移量、速度）
        double speed = 1.0;          // 速度补偿（默认1.0，无补偿）
        int offset = 0;              // 时间偏移量（帧单位，正数表示延后）
        double quality;              // 对齐质量（0-1，值越高对齐越准确）
        double speedRange = Settings.audioReferenceSpeedRange();  // 用户设置的速度调整范围

        if (speedRange != 0.0) {
            // 如果启用速度调整：计算偏移量+速度补偿（允许音频轻微变速以对齐）
            quality = m_referenceArray.calculateOffsetAndSpeed(m_clipArray,
                                                               &speed,
                                                               &offset,
                                                               speedRange);
        } else {
            // 如果禁用速度调整：仅计算偏移量（音频速度固定）
            quality = m_referenceArray.calculateOffset(m_clipArray, &offset);
        }

        // 4. 发送完成进度（100%）
        onReaderProgressUpdate(100);
        // 5. 发送处理完成信号：携带剪辑索引、对齐参数、质量
        emit finished(m_index, offset, speed, quality);
    }

public slots:
    // 接收AudioReader的进度信号，转换后转发（将0-100映射为0-99，留1%给最终步骤）
    void onReaderProgressUpdate(int progress)
    {
        progress = progress * 99 / 100;  // 调整进度范围，避免提前显示100%
        emit progressUpdate(m_index, progress);  // 转发进度（带剪辑索引）
    }

signals:
    // 进度更新信号：参数为剪辑索引、当前进度（0-99）
    void progressUpdate(int index, int percent);
    // 处理完成信号：参数为剪辑索引、偏移量、速度、对齐质量
    void finished(int index, int offset, double speed, double quality);

private:
    AlignmentArray m_clipArray;        // 存储当前剪辑的音频特征数组
    AlignmentArray &m_referenceArray;  // 参考轨道的音频特征数组（引用）
    AudioReader m_reader;              // 音频读取器（处理当前剪辑音频）
    int m_index;                       // 剪辑在列表中的索引
    QFuture<void> m_future;            // 异步任务对象（管理多线程执行）
    bool m_calculateSpeed;             // （未使用）是否计算速度补偿的标记
};


// 【 内部辅助类】：AlignTableDelegate
// 【功能】：自定义QTreeView的单元格绘制逻辑（显示进度条、状态图标等）
class AlignTableDelegate : public QStyledItemDelegate
{
    Q_OBJECT  // 启用Qt信号槽机制
public:
    // 重写绘制函数：自定义单元格显示内容
    // painter：绘图工具
    // option：单元格样式选项（如位置、大小）
    // index：单元格的模型索引（标识行/列）
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        // 将模型索引转换为AlignClipsModel（安全转换，获取自定义模型数据）
        const AlignClipsModel *model = dynamic_cast<const AlignClipsModel *>(index.model());
        
        // 根据列索引，自定义不同的绘制逻辑
        switch (index.column()) {
        // 1. 状态列（COLUMN_ERROR）：显示错误/完成图标
        case AlignClipsModel::COLUMN_ERROR: {
            QIcon icon;  // 状态图标
            // 如果有错误信息：显示"任务拒绝"图标（红色叉号）
            if (!index.data().toString().isEmpty()) {
                icon = QIcon(":/icons/oxygen/32x32/status/task-reject.png");
            }
            // 如果处理完成（进度100%）：显示"任务完成"图标（绿色对勾）
            else if (model->getProgress(index.row()) == 100) {
                icon = QIcon(":/icons/oxygen/32x32/status/task-complete.png");
            }
            // 绘制图标（居中显示）
            icon.paint(painter, option.rect, Qt::AlignCenter);
            break;
        }

        // 2. 剪辑名称列（COLUMN_NAME）：显示名称+进度条
        case AlignClipsModel::COLUMN_NAME: {
            int progress = model->getProgress(index.row());  // 获取当前行的进度
            if (progress > 0) {
                // 绘制进度条（覆盖单元格背景）
                QStyleOptionProgressBar progressBarOption;
                progressBarOption.rect = option.rect;       // 进度条位置（与单元格一致）
                progressBarOption.minimum = 0;              // 进度最小值
                progressBarOption.maximum = 100;            // 进度最大值
                progressBarOption.progress = progress;      // 当前进度
                // 使用系统样式绘制进度条
                QApplication::style()->drawControl(QStyle::CE_ProgressBar,
                                                   &progressBarOption,
                                                   painter);
            }
            // 在进度条上方绘制剪辑名称（左对齐、垂直居中）
            painter->drawText(option.rect,
                              Qt::AlignLeft | Qt::AlignVCenter,
                              index.data().toString());
            break;
        }

        // 3. 偏移量列（COLUMN_OFFSET）和速度列（COLUMN_SPEED）：使用默认绘制
        case AlignClipsModel::COLUMN_OFFSET:
        case AlignClipsModel::COLUMN_SPEED:
            QStyledItemDelegate::paint(painter, option, index);
            break;

        // 4. 无效列：打印错误日志
        default:
            LOG_ERROR() << "Invalid Column" << index.row() << index.column();
            break;
        }
    }
};

// 包含MOC文件（Qt元对象编译器生成的信号槽代码），因AlignTableDelegate在.cpp中定义
#include "alignaudiodialog.moc"


// 【 AlignAudioDialog 核心类实现】
// 构造函数：初始化音频对齐对话框的UI布局和数据
// title：对话框标题
// model：多轨道数据模型（管理时间线剪辑）
// uuids：需要对齐的剪辑UUID列表
// parent：父窗口指针
AlignAudioDialog::AlignAudioDialog(QString title,
                                   MultitrackModel *model,
                                   const QVector<QUuid> &uuids,
                                   QWidget *parent)
    : QDialog(parent)
    , m_model(model)          // 保存多轨道模型指针
    , m_uuids(uuids)          // 保存需要对齐的剪辑UUID
    , m_uiTask(nullptr)       // 初始化长时间任务对象（空）
{
    int row = 0;  // 网格布局的行索引（用于排版）

    // 1. 基础窗口设置
    setWindowTitle(title);  // 设置对话框标题
    // 设置模态类型（与QML应用保持一致，阻塞父窗口操作）
    setWindowModality(QmlApplication::dialogModality());

    // 2. 创建网格布局（管理UI控件位置）
    QGridLayout *glayout = new QGridLayout();
    glayout->setHorizontalSpacing(4);  // 水平控件间距
    glayout->setVerticalSpacing(2);    // 垂直控件间距

    // 3. 第一行：参考音频轨道下拉框（选择哪个轨道作为对齐基准）
    // 3.1 添加标签（文本："Reference audio track"，右对齐）
    glayout->addWidget(new QLabel(tr("Reference audio track")), row, 0, Qt::AlignRight);
    // 3.2 创建下拉框（m_trackCombo）
    m_trackCombo = new QComboBox();
    int trackCount = m_model->trackList().size();  // 获取轨道总数
    // 为下拉框添加所有轨道选项（文本：轨道名称，数据：轨道索引）
    for (int i = 0; i < trackCount; i++) {
        m_trackCombo->addItem(m_model->getTrackName(i), QVariant(i));
    }
    // 加载用户默认的参考轨道（从配置中读取）
    int defaultTrack = Settings.audioReferenceTrack();
    if (defaultTrack < trackCount) {
        m_trackCombo->setCurrentIndex(defaultTrack);
    }
    // 连接下拉框选择变化信号到"重建剪辑列表"槽函数（兼容Qt新旧信号语法）
    if (!connect(m_trackCombo,
                 QOverload<int>::of(&QComboBox::activated),
                 this,
                 &AlignAudioDialog::rebuildClipList))
        connect(m_trackCombo, SIGNAL(activated(const QString &)), SLOT(rebuildClipList()));
    // 将下拉框添加到布局（第row行，第1列，左对齐）
    glayout->addWidget(m_trackCombo, row++, 1, Qt::AlignLeft);

    // 4. 第二行：速度调整范围下拉框（选择对齐时允许的速度补偿范围）
    // 4.1 添加标签（文本："Speed adjustment range"，右对齐）
    glayout->addWidget(new QLabel(tr("Speed adjustment range")), row, 0, Qt::AlignRight);
    // 4.2 创建下拉框（m_speedCombo）
    m_speedCombo = new QComboBox();
    // 设置提示 tooltip（说明范围越大处理时间越长）
    m_speedCombo->setToolTip("Larger speed adjustment ranges take longer to process.");
    // 添加选项：文本显示范围描述，数据存储实际范围值（0表示无速度调整）
    m_speedCombo->addItem(tr("None") + QStringLiteral(" (%L1%)").arg(0), QVariant(0));
    m_speedCombo->addItem(tr("Narrow") + QStringLiteral(" (%L1%)").arg((double) 0.1, 0, 'g', 2),
                          QVariant(0.001));  // 0.1%范围（实际值0.001）
    m_speedCombo->addItem(tr("Normal") + QStringLiteral(" (%L1%)").arg((double) 0.5, 0, 'g', 2),
                          QVariant(0.005));  // 0.5%范围
    m_speedCombo->addItem(tr("Wide") + QStringLiteral(" (%L1%)").arg(1),
                          QVariant(0.01));   // 1%范围
    m_speedCombo->addItem(tr("Very wide") + QStringLiteral(" (%L1%)").arg(5),
                          QVariant(0.05));   // 5%范围
    // 加载用户默认的速度范围（从配置中读取）
    double defaultRange = Settings.audioReferenceSpeedRange();
    for (int i = 0; i < m_speedCombo->count(); i++) {
        if (m_speedCombo->itemData(i).toDouble() == defaultRange) {
            m_speedCombo->setCurrentIndex(i);
            break;
        }
    }
    // 连接下拉框选择变化信号到"重建剪辑列表"槽函数
    if (!connect(m_speedCombo,
                 QOverload<int>::of(&QComboBox::activated),
                 this,
                 &AlignAudioDialog::rebuildClipList))
        connect(m_speedCombo, SIGNAL(activated(const QString &)), SLOT(rebuildClipList()));
    // 将下拉框添加到布局
    glayout->addWidget(m_speedCombo, row++, 1, Qt::AlignLeft);

    // 5. 第三行：剪辑列表视图（QTreeView，显示需要对齐的剪辑信息）
    m_table = new QTreeView();
    // 配置视图属性
    m_table->setSelectionMode(QAbstractItemView::NoSelection);  // 禁用选择（仅显示）
    m_table->setItemsExpandable(false);                         // 禁用项目展开
    m_table->setRootIsDecorated(false);                         // 禁用根节点装饰（无折叠图标）
    m_table->setUniformRowHeights(true);                        // 所有行高度一致（优化绘制）
    m_table->setSortingEnabled(false);                          // 禁用排序
    m_table->setModel(&m_alignClipsModel);                      // 设置数据模型（显示剪辑数据）
    m_table->setWordWrap(false);                                // 禁用文本换行
    // 设置自定义委托（用于绘制进度条、状态图标）
    m_delegate = new AlignTableDelegate();
    m_table->setItemDelegate(m_delegate);
    // 配置表头（列宽和拉伸规则）
    m_table->header()->setStretchLastSection(false);  // 最后一列不拉伸
    // 计算行高（根据字体高度和设备像素比）
    qreal rowHeight = fontMetrics().height() * devicePixelRatioF();
    m_table->header()->setMinimumSectionSize(rowHeight);  // 列最小宽度
    // 状态列（COLUMN_ERROR）：固定宽度（与行高一致）
    m_table->header()->setSectionResizeMode(AlignClipsModel::COLUMN_ERROR, QHeaderView::Fixed);
    m_table->setColumnWidth(AlignClipsModel::COLUMN_ERROR, rowHeight);
    // 剪辑名称列（COLUMN_NAME）：拉伸填充剩余空间
    m_table->header()->setSectionResizeMode(AlignClipsModel::COLUMN_NAME, QHeaderView::Stretch);
    // 偏移量列（COLUMN_OFFSET）：固定宽度（足够显示时间格式如"00:00:00:00"）
    m_table->header()->setSectionResizeMode(AlignClipsModel::COLUMN_OFFSET, QHeaderView::Fixed);
    m_table->setColumnWidth(AlignClipsModel::COLUMN_OFFSET,
                            fontMetrics().horizontalAdvance("-00:00:00:00") * devicePixelRatioF()
                                + 8);  // 加8像素边距
    // 速度列（COLUMN_SPEED）：根据内容自动调整宽度
    m_table->header()->setSectionResizeMode(AlignClipsModel::COLUMN_SPEED,
                                            QHeaderView::ResizeToContents);
    // 将视图添加到布局（跨2列，占1行）
    glayout->addWidget(m_table, row++, 0, 1, 2);

    // 6. 第四行：按钮组（取消、处理、应用、处理并应用）
    // 6.1 创建按钮组（默认包含"取消"按钮）
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    // 连接"取消"按钮信号到对话框的拒绝槽函数（关闭对话框）
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    // 将按钮组添加到布局（跨2列）
    glayout->addWidget(m_buttonBox, row++, 0, 1, 2);

    // 6.2 添加"处理"按钮（ActionRole：自定义功能按钮）
    QPushButton *processButton = m_buttonBox->addButton(tr("Process"), QDialogButtonBox::ActionRole);
    connect(processButton, SIGNAL(pressed()), this, SLOT(process()));  // 点击触发处理逻辑

    // 6.3 添加"应用"按钮（ApplyRole：应用结果按钮）
    m_applyButton = m_buttonBox->addButton(tr("Apply"), QDialogButtonBox::ApplyRole);
    connect(m_applyButton, SIGNAL(pressed()), this, SLOT(apply()));  // 点击触发应用逻辑
    m_applyButton->setEnabled(false);  // 初始禁用（未处理时无法应用）

    // 6.4 添加"处理并应用"按钮（AcceptRole：确认按钮，优先级高）
    m_processAndApplyButton = m_buttonBox->addButton(tr("Process + Apply"),
                                                     QDialogButtonBox::AcceptRole);
    connect(m_processAndApplyButton, SIGNAL(pressed()), this, SLOT(processAndApply()));  // 处理+应用

    // 7. 设置对话框布局并启用模态
    this->setLayout(glayout);
    this->setModal(true);  // 模态对话框：阻塞父窗口操作

    // 8. 初始化剪辑列表（根据当前参考轨道和速度范围）
    rebuildClipList();

    // 9. 设置对话框初始大小
    resize(500, 300);
}

// 析构函数：释放内存（自定义委托和长时间任务对象）
AlignAudioDialog::~AlignAudioDialog()
{
    delete m_delegate;   // 释放自定义委托对象
    delete m_uiTask;     // 释放长时间任务对象
}

// 槽函数：重建剪辑列表（当参考轨道或速度范围变化时调用）
void AlignAudioDialog::rebuildClipList()
{
    // 1. 清空现有列表数据
    m_alignClipsModel.clear();

    // 2. 保存当前用户设置（参考轨道索引、速度范围）
    int referenceIndex = m_trackCombo->currentData().toInt();
    Settings.setAudioReferenceTrack(referenceIndex);  // 保存参考轨道到配置
    Settings.setAudioReferenceSpeedRange(m_speedCombo->currentData().toDouble());  // 保存速度范围

    // 3. 禁用"应用"按钮（列表重建后需重新处理）
    m_applyButton->setEnabled(false);

    // 4. 遍历需要对齐的剪辑UUID，添加到列表
    for (const auto &uuid : m_uuids) {
        int trackIndex, clipIndex;  // 剪辑所在的轨道索引、轨道内剪辑索引
        // 根据UUID查找剪辑信息（返回剪辑的MLT生产者、入点/出点等）
        auto info = m_model->findClipByUuid(uuid, trackIndex, clipIndex);
        
        // 如果剪辑有效（存在且MLT剪切对象合法）
        if (info && info->cut && info->cut->is_valid()) {
            QString error;  // 错误信息（如不支持对齐、在参考轨道上）
            QString clipName;  // 剪辑名称（用于显示）

            // 4.1 获取剪辑名称（优先使用Shotcut自定义标题属性）
            clipName = info->producer->get(kShotcutCaptionProperty);
            // 如果无标题，使用资源文件名（去除路径）
            if (clipName.isNull() || clipName.isEmpty())
                clipName = Util::baseName(ProxyManager::resource(*info->producer));
            // 如果仍无名称，使用MLT服务类型（如"avformat"）
            if (clipName == "<producer>" || clipName.isNull() || clipName.isEmpty())
                clipName = QString::fromUtf8(info->producer->get("mlt_service"));

            // 4.2 检查是否需要跳过当前剪辑
            // 情况1：剪辑在参考轨道上（无需对齐自身）
            if (trackIndex == referenceIndex) {
                error = tr("This clip will be skipped because it is on the reference track.");
            }
            // 情况2：剪辑不是avformat类型（仅支持avformat格式的音频对齐）
            else {
                // 获取Shotcut生产者类型和MLT服务类型
                QString shotcutProducer(info->producer->get(kShotcutProducerProperty));
                QString service(info->producer->get("mlt_service"));
                // 若两者都不是avformat，标记为不支持对齐
                if (!service.startsWith("avformat") && !shotcutProducer.startsWith("avformat"))
                    error = tr("This item can not be aligned.");
            }

            // 4.3 将剪辑添加到数据模型（初始偏移量和速度为无效值，带错误信息）
            m_alignClipsModel.addClip(clipName,
                                      AlignClipsModel::INVALID_OFFSET,  // 初始无效偏移量
                                      AlignClipsModel::INVALID_OFFSET,  // 初始无效速度
                                      error);  // 错误信息（空表示正常）
        }
    }
}

// 槽函数：处理音频对齐（核心逻辑）
void AlignAudioDialog::process()
{
    // 1. 创建长时间任务对象（显示处理进度窗口）
    m_uiTask = new LongUiTask(tr("Align Audio"));
    m_uiTask->setMinimumDuration(0);  // 立即显示进度窗口（无延迟）

    // 2. 获取参考轨道信息并创建MLT轨道生产者（读取参考轨道音频）
    int referenceTrackIndex = m_trackCombo->currentData().toInt();  // 参考轨道索引
    auto mlt_index = m_model->trackList().at(referenceTrackIndex).mlt_index;  // MLT轨道索引
    // 创建参考轨道的MLT生产者（智能指针管理）
    QScopedPointer<Mlt::Producer> track(m_model->tractor()->track(mlt_index));
    int maxLength = track->get_playtime();  // 参考轨道总帧数（作为特征数组最大长度）
    bool validClip = false;  // 是否存在可对齐的有效剪辑（避免空处理）
    QString xml = MLT.XML(track.data());  // 参考轨道的MLT XML描述

    // 3. 初始化参考轨道音频处理（读取特征数据）
    AlignmentArray trackArray;  // 参考轨道的音频特征数组
    AudioReader trackReader(MLT.XML(track.data()), &trackArray);  // 参考轨道音频读取器
    // 连接参考轨道进度信号到更新槽函数（显示参考轨道分析进度）
    connect(&trackReader, SIGNAL(progressUpdate(int)), this, SLOT(updateReferenceProgress(int)));

    // 4. 初始化所有待对齐剪辑的音频读取器
    QList<ClipAudioReader *> m_clipReaders;  // 剪辑音频读取器列表
    for (const auto &uuid : m_uuids) {
        int trackIndex, clipIndex;  // 剪辑所在轨道/剪辑索引
        auto info = m_model->findClipByUuid(uuid, trackIndex, clipIndex);  // 查找剪辑信息

        // 跳过无效剪辑（不存在或MLT剪切对象非法）
        if (!info || !info->cut || !info->cut->is_valid()) {
            m_clipReaders.append(nullptr);  // 用nullptr标记无效剪辑
            continue;
        }

        // 检查剪辑是否支持对齐（avformat类型）
        QString shotcutProducer(info->producer->get(kShotcutProducerProperty));
        QString service(info->producer->get("mlt_service"));
        // 情况1：不支持对齐（非avformat）或在参考轨道上：标记为无效
        if (!service.startsWith("avformat") && !shotcutProducer.startsWith("avformat")) {
            m_clipReaders.append(nullptr);
        } else if (trackIndex == referenceTrackIndex) {
            m_clipReaders.append(nullptr);
        }
        // 情况2：支持对齐：创建ClipAudioReader并配置
        else {
            QString xml = MLT.XML(info->cut);  // 剪辑的MLT XML描述
            // 创建剪辑音频读取器（传入参考轨道特征数组、剪辑索引、入点/出点）
            ClipAudioReader *clipReader = new ClipAudioReader(xml,
                                                              trackArray,
                                                              m_clipReaders.size(),  // 剪辑在列表中的索引
                                                              info->frame_in,        // 剪辑入点
                                                              info->frame_out);      // 剪辑出点
            // 连接剪辑进度信号（更新列表进度条）
            connect(clipReader,
                    SIGNAL(progressUpdate(int, int)),
                    this,
                    SLOT(updateClipProgress(int, int)));
            // 连接剪辑完成信号（更新对齐参数）
            connect(clipReader,
                    SIGNAL(finished(int, int, double, double)),
                    this,
                    SLOT(clipFinished(int, int, double, double)));
            // 添加到读取器列表
            m_clipReaders.append(clipReader);
            // 更新最大长度（取参考轨道和当前剪辑的最大帧数，避免数组越界）
            maxLength = qMax(maxLength, info->frame_count);
            validClip = true;  // 标记存在有效剪辑
        }
    }

    // 5. 如果无有效剪辑：释放资源并返回
    if (!validClip) {
        m_uiTask->deleteLater();  // 释放进度窗口
        m_uiTask = nullptr;
        return;
    }

    // 6. 初始化参考轨道和所有剪辑的特征数组（设置最大长度）
    trackReader.init(maxLength);
    for (const auto &clipReader : m_clipReaders) {
        if (clipReader)  // 仅初始化有效读取器
            clipReader->init(maxLength);
    }

    // 7. 处理参考轨道音频（生成特征数组）
    trackReader.process();

    // 8. 启动所有剪辑的音频处理（异步多线程执行）
    for (const auto &clipReader : m_clipReaders) {
        if (clipReader)
            clipReader->start();
    }

    // 9. 等待所有剪辑处理完成（主线程循环检查，同时处理UI事件）
    for (const auto &clipReader : m_clipReaders) {
        if (clipReader) {
            // 循环等待，直到处理完成（每10ms检查一次，避免阻塞UI）
            while (!clipReader->isFinished()) {
                QThread::msleep(10);          // 休眠10ms（减少CPU占用）
                QCoreApplication::processEvents();  // 处理UI事件（保持界面响应）
            }
            clipReader->deleteLater();  // 处理完成后释放读取器
        }
    }

    // 10. 处理完成：释放进度窗口，启用"应用"按钮
    m_uiTask->deleteLater();
    m_uiTask = nullptr;
    m_applyButton->setEnabled(true);
}

// 槽函数：应用对齐结果（将计算出的偏移量和速度应用到时间线剪辑）
void AlignAudioDialog::apply()
{
    // 1. 创建音频对齐命令（用于撤销/重做，遵循命令模式）
    Timeline::AlignClipsCommand *command = new Timeline::AlignClipsCommand(*m_model);
    int referenceTrackIndex = m_trackCombo->currentData().toInt();  // 参考轨道索引
    int alignmentCount = 0;  // 实际对齐的剪辑数量
    int modelIndex = 0;      // 剪辑在数据模型中的索引

    // 2. 遍历所有待对齐的剪辑UUID，应用对齐参数
    for (const auto &uuid : m_uuids) {
        int trackIndex, clipIndex;  // 剪辑所在轨道/剪辑索引
        auto info = m_model->findClipByUuid(uuid, trackIndex, clipIndex);  // 查找剪辑信息

        // 跳过无效剪辑或参考轨道上的剪辑
        if (!info || !info->cut || !info->cut->is_valid()) {
            modelIndex++;  // 即使无效，模型索引也要递增（保持对应关系）
            continue;
        }

        // 仅处理非参考轨道的剪辑
        if (trackIndex != referenceTrackIndex) {
            // 从数据模型中获取当前剪辑的对齐参数（偏移量）
            int offset = m_alignClipsModel.getOffset(modelIndex);
            // 如果偏移量有效（已计算出对齐结果）
            if (offset != AlignClipsModel::INVALID_OFFSET) {
                // 获取速度补偿值
                double speedCompensation = m_alignClipsModel.getSpeed(modelIndex);
                // 将对齐参数添加到命令中（UUID标识剪辑，偏移量和速度为参数）
                command->addAlignment(uuid, offset, speedCompensation);
                alignmentCount++;  // 递增对齐计数
            }
        }
        modelIndex++;  // 模型索引递增
    }

    // 3. 执行命令（添加到撤销栈，支持后续撤销）
    if (alignmentCount > 0) {
        MAIN.undoStack()->push(command);  // 将命令压入主窗口的撤销栈
    } else {
        delete command;  // 无有效对齐：释放命令对象（避免内存泄漏）
    }

    // 4. 关闭对话框（应用完成）
    accept();
}

// 槽函数：处理并应用（先执行process，再执行apply）
void AlignAudioDialog::processAndApply()
{
    process();  // 先处理音频对齐
    apply();    // 再应用对齐结果
}

// 槽函数：更新参考轨道处理进度（显示在LongUiTask窗口）
void AlignAudioDialog::updateReferenceProgress(int percent)
{
    if (m_uiTask) {
        // 报告进度：文本描述"分析参考轨道"，当前进度percent，总进度100
        m_uiTask->reportProgress(tr("Analyze Reference Track"), percent, 100);
    }
}

// 槽函数：更新单个剪辑的处理进度（显示在列表进度条）
void AlignAudioDialog::updateClipProgress(int index, int percent)
{
    // 更新数据模型中对应剪辑的进度（触发视图刷新进度条）
    m_alignClipsModel.updateProgress(index, percent);
    if (m_uiTask) {
        // 报告剪辑分析进度（文本描述"分析剪辑"，不更新百分比数值）
        m_uiTask->reportProgress(tr("Analyze Clips"), 0, 0);
    }
}

// 槽函数：单个剪辑处理完成（更新对齐参数和状态）
void AlignAudioDialog::clipFinished(int index, int offset, double speed, double quality)
{
    QString error;  // 错误信息（如对齐失败）

    // 打印日志：剪辑索引、对齐参数、质量
    LOG_INFO() << "Clip" << index << "Offset:" << offset << "Speed:" << speed
               << "Quality:" << quality;

    // 检查对齐质量：若质量低于0.01（1%），判定为对齐失败
    if (quality < 0.01) {
        error = tr("Alignment not found.");  // 错误信息：未找到对齐位置
        offset = AlignClipsModel::INVALID_OFFSET;  // 标记偏移量为无效
        speed = AlignClipsModel::INVALID_OFFSET;   // 标记速度为无效
    }

    // 更新数据模型中对应剪辑的对齐参数和错误信息
    m_alignClipsModel.updateOffsetAndSpeed(index, offset, speed, error);
    // 更新进度为100%（处理完成）
    m_alignClipsModel.updateProgress(index, 100);
}
