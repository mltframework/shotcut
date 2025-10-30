/*
 * Copyright (c) 2025 Meltytech, LLC
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
#include "speechdialog.h"

// 引入依赖的业务逻辑头文件
#include "Logger.h"                  // 日志工具类，用于打印调试信息
#include "mltcontroller.h"           // MLT控制器（获取视频配置、创建音频生产者/消费者）
#include "qmltypes/qmlapplication.h" // QML应用类（获取数据目录、对话框模态属性）
#include "settings.h"                // 配置类（读取/保存语音语言、声音等偏好设置）

// 引入Qt相关头文件
#include <QComboBox>                 // 下拉选择框（用于选择语言、声音）
#include <QDialogButtonBox>          // 对话框按钮组（后续可扩展确定/取消按钮）
#include <QDoubleSpinBox>            // 双精度数值输入框（预留，如语速调节）
#include <QFileDialog>               // 文件对话框（预留，如导出语音文件）
#include <QGridLayout>               // 网格布局管理器（排列界面控件）
#include <QHBoxLayout>               // 水平布局管理器（排列声音选择框和预览按钮）
#include <QLabel>                    // 标签控件（显示文本提示）
#include <QLineEdit>                 // 单行输入框（预留，如输入文本内容）
#include <QPushButton>               // 按钮控件（语音预览按钮）
#include <QStringList>               // 字符串列表（存储语言、声音选项）


// 【构造函数】：初始化文本转语音对话框
// 参数：parent - 父窗口指针
SpeechDialog::SpeechDialog(QWidget *parent)
{
    setWindowTitle(tr("Text to Speech"));  // 设置对话框标题（“文本转语音”）
    // 设置对话框模态属性（与QML应用保持一致，确保阻塞父窗口操作）
    setWindowModality(QmlApplication::dialogModality());

    // 1. 创建网格布局（管理对话框内所有控件的排列）
    auto grid = new QGridLayout;
    setLayout(grid);
    // 固定对话框大小（不可拉伸，避免控件布局错乱）
    grid->setSizeConstraint(QLayout::SetFixedSize);

    // 2. 语言选择行（标签 + 下拉框）
    // 创建语言标签（右对齐，与下拉框对齐）
    auto languageLabel = new QLabel(tr("Language"), this);
    // 创建语言下拉框（存储语言选项）
    m_language = new QComboBox(this);
    // 向下拉框添加语言选项：文本显示用户可见名称，数据存储语言标识（如“a”代表美式英语）
    m_language->addItem(tr("American English"), QStringLiteral("a"));    // 美式英语
    m_language->addItem(tr("British English"), QStringLiteral("b"));     // 英式英语
    m_language->addItem(tr("Spanish"), QStringLiteral("e"));             // 西班牙语
    m_language->addItem(tr("French"), QStringLiteral("f"));              // 法语
    m_language->addItem(tr("Hindi"), QStringLiteral("h"));               // 印地语
    m_language->addItem(tr("Italian"), QStringLiteral("i"));             // 意大利语
    m_language->addItem(tr("Portuguese"), QStringLiteral("p"));          // 葡萄牙语
    m_language->addItem(tr("Japanese"), QStringLiteral("j"));            // 日语
    m_language->addItem(tr("Mandarin Chinese"), QStringLiteral("z"));    // 普通话中文

    // 从配置中读取已保存的语言选择，自动选中对应选项
    const QString savedLang = Settings.speechLanguage();
    for (int i = 0; i < m_language->count(); ++i) {
        // 匹配下拉框选项的存储数据与配置中的语言标识
        if (m_language->itemData(i).toString() == savedLang) {
            m_language->setCurrentIndex(i);
            break;
        }
    }

    // 将语言标签和下拉框添加到网格布局（第0行：标签在第0列，下拉框在第1列）
    grid->addWidget(languageLabel, 0, 0, Qt::AlignRight);
    grid->addWidget(m_language, 0, 1);

    // 3. 声音选择行（标签 + 下拉框 + 预览按钮）
    // 创建声音标签（右对齐）
    auto voiceLabel = new QLabel(tr("Voice"), this);
    grid->addWidget(voiceLabel, 1, 0, Qt::AlignRight);  // 添加到网格第1行第0列

    // 创建声音下拉框（设置最小宽度，确保显示完整）
    m_voice = new QComboBox(this);
    m_voice->setMinimumWidth(120);

    // 创建声音预览按钮（使用系统主题图标，无文本，鼠标悬浮显示提示）
    auto icon = QIcon::fromTheme("media-playback-start",
                                 QIcon(":/icons/oxygen/32x32/actions/media-playback-start.png"));
    auto voiceButton = new QPushButton(icon, QString(), this);
    voiceButton->setToolTip(tr("Preview this voice"));  // 预览按钮提示文本

    // 创建水平布局，包裹声音下拉框和预览按钮（避免控件分散）
    auto voiceRow = new QWidget(this);
    auto voiceLayout = new QHBoxLayout(voiceRow);
    voiceLayout->setContentsMargins(0, 0, 0, 0);  // 取消布局边距
    voiceLayout->setSpacing(4);                   // 控件间距4像素
    voiceLayout->addWidget(m_voice);              // 添加声音下拉框
    voiceLayout->addWidget(voiceButton);          // 添加预览按钮
    voiceLayout->addStretch();                    // 拉伸空间，将控件靠左对齐

    // 将声音选择行添加到网格布局（第1行，跨第1-2列，避免与其他列重叠）
    grid->addWidget(voiceRow, 1, 1, 1, 2);

    // 4. 连接预览按钮点击信号（播放选中的声音文件）
    connect(voiceButton, &QPushButton::clicked, this, [this]() {
        // 1. 获取声音文件所在目录（QML应用数据目录下的“shotcut/voices”）
        auto dir = QmlApplication::dataDir();
        dir.cd("shotcut");
        dir.cd("voices");
        // 2. 拼接声音文件路径：目录 + 选中声音的标识 + ".opus"（声音文件格式）
        const auto filename = dir.filePath(m_voice->currentData().toString().append(".opus"));
        LOG_DEBUG() << filename;  // 打印声音文件路径（调试用）

        // 3. 创建MLT音频生产者（加载声音文件）
        Mlt::Producer p(MLT.profile(), filename.toLocal8Bit().constData());
        
        // 4. 停止当前正在播放的音频（避免多音频叠加）
        if (m_consumer && m_consumer->is_valid())
            m_consumer->stop();

        // 5. 创建MLT音频消费者（使用SDL2音频输出，仅播放音频，不显示视频）
        m_consumer.reset(new Mlt::Consumer(MLT.profile(), "sdl2_audio"));
        if (!m_consumer || !m_consumer->is_valid())
            return;  // 消费者创建失败则返回

        // 6. 连接生产者与消费者，配置播放参数并开始播放
        m_consumer->connect(p);
        m_consumer->set("terminate_on_pause", 1);  // 暂停时终止播放
        m_consumer->set("video_off", 1);           // 关闭视频输出（仅音频）
        m_consumer->start();                       // 开始播放音频
    });

    // 5. 初始化声音选项（根据初始选中的语言加载对应声音）
    populateVoices(m_language->currentData().toString());
    // 6. 连接语言选择变化信号（切换语言时，重新加载对应声音选项）
    connect(m_language, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        populateVoices(m_language->currentData().toString());
    });

    // 7. 从配置中读取已保存的声音选择，自动选中对应选项
    const QString savedVoice = Settings.speechVoice();
    if (!savedVoice.isEmpty()) {
        for (int i = 0; i < m_voice->count(); ++i) {
            // 匹配下拉框选项的存储数据与配置中的声音标识
            if (m_voice->itemData(i).toString() == savedVoice) {
                m_voice->setCurrentIndex(i);
                break;
            }
        }
    }
}

    // 【控件布局】：语速选择行（标签 + 双精度数值输入框）
    auto speedLabel = new QLabel(tr("Speed"), this);  // 语速标签（显示“语速”）
    m_speed = new QDoubleSpinBox(this);               // 语速输入框（双精度，支持小数调节）
    m_speed->setRange(0.5, 2.0);                      // 设置语速范围：0.5倍（慢）~2.0倍（快）
    m_speed->setDecimals(2);                          // 设置显示小数位数：2位（如1.05、1.50）
    m_speed->setSingleStep(0.05);                     // 设置步长：每次点击增减0.05倍
    m_speed->setValue(Settings.speechSpeed());        // 初始值：从配置中读取已保存的语速

    // 创建水平布局包裹语速输入框（避免与其他控件错位）
    auto speedRow = new QWidget(this);
    auto speedLayout = new QHBoxLayout(speedRow);
    speedLayout->setContentsMargins(0, 0, 0, 0);  // 取消布局边距
    speedLayout->setSpacing(4);                   // 控件间距4像素
    speedLayout->addWidget(m_speed);              // 添加语速输入框
    speedLayout->addStretch();                    // 拉伸空间，将输入框靠左对齐

    // 将语速标签和输入框行添加到网格布局（第2行：标签在0列右对齐，输入框行跨1-2列）
    grid->addWidget(speedLabel, 2, 0, Qt::AlignRight);
    grid->addWidget(speedRow, 2, 1, 1, 2);


    // 【控件布局】：输出文件行（标签 + 输入框 + 浏览按钮）
    auto outputLabel = new QLabel(tr("Output file"), this);  // 输出文件标签（显示“输出文件”）
    m_outputFile = new QLineEdit(this);                      // 输出文件路径输入框
    m_outputFile->setMinimumWidth(300);                      // 设置输入框最小宽度（确保显示完整路径）
    m_outputFile->setPlaceholderText(tr("Click the button to set the file"));  // 占位提示文本
    m_outputFile->setDisabled(true);                         // 设置输入框禁用（只能通过浏览按钮修改）
    // 初始值：从QML应用获取默认文件名（格式“speech-xxx.wav”，避免重复）
    m_outputFile->setText(QmlApplication::getNextProjectFile("speech-.wav"));

    // 创建“浏览”按钮（使用系统主题图标或本地图标，无文本）
    icon = QIcon::fromTheme("document-save",
                            QIcon(":/icons/oxygen/32x32/actions/document-save.png"));
    auto browseButton = new QPushButton(icon, QString(), this);

    // 将输出文件标签添加到网格布局（第3行0列，右对齐）
    grid->addWidget(outputLabel, 3, 0, Qt::AlignRight);

    // 创建水平布局包裹输出输入框和浏览按钮
    auto outputRow = new QWidget(this);
    auto outputLayout = new QHBoxLayout(outputRow);
    outputLayout->setContentsMargins(0, 0, 0, 0);  // 取消布局边距
    outputLayout->setSpacing(4);                   // 控件间距4像素
    outputLayout->addWidget(m_outputFile);         // 添加输出路径输入框
    outputLayout->addWidget(browseButton);         // 添加浏览按钮

    // 将输出文件行添加到网格布局（第3行，跨1-2列）
    grid->addWidget(outputRow, 3, 1, 1, 2);


    // 【信号槽连接】：浏览按钮点击事件（选择输出文件路径）
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        // 打开文件保存对话框：标题“保存音频文件”，默认路径为配置中的保存目录，过滤.wav文件
        const QString selected = QFileDialog::getSaveFileName(this,
                                                              tr("Save Audio File"),
                                                              Settings.savePath(),
                                                              tr("WAV files (*.wav)"));
        if (!selected.isEmpty()) {  // 若用户选择了路径
            QString path = selected;
            // 若路径未以.wav结尾（不区分大小写），自动添加.wav后缀
            if (!path.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive)) {
                path += QStringLiteral(".wav");
            }
            m_outputFile->setText(path);  // 更新输入框显示的路径
            // 将当前选择的目录保存到配置（作为下次默认目录）
            Settings.setSavePath(QFileInfo(selected).path());
        }
    });


    // 【控件布局】：按钮组（确定 + 取消按钮）
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    // 将按钮组添加到网格布局（第4行，跨0-2列，居中显示）
    grid->addWidget(buttonBox, 4, 0, 1, 3);

    // 【信号槽连接】：取消按钮点击事件（关闭对话框）
    connect(buttonBox->button(QDialogButtonBox::Cancel),
            &QAbstractButton::clicked,
            this,
            &QDialog::close);

    // 【信号槽连接】：确定按钮点击事件（保存配置 + 触发语音生成）
    connect(buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, [&] {
        // 1. 获取当前用户选择的参数
        const QString lang = m_language->currentData().toString();    // 语言标识（如“a”=美式英语）
        const QString voice = m_voice->currentData().toString();      // 声音标识（对应具体语音文件）
        const double speed = m_speed->value();                        // 语速（0.5~2.0倍）

        // 2. 处理输出文件路径（确保路径有效）
        QString path = m_outputFile->text().trimmed();
        if (path.isEmpty()) {  // 若未选择路径，重新弹出文件选择对话框
            LOG_DEBUG() << Settings.savePath();
            const QString selected = QFileDialog::getSaveFileName(this,
                                                                  tr("Save Audio File"),
                                                                  Settings.savePath(),
                                                                  tr("WAV files (*.wav)"));
            if (selected.isEmpty()) {
                return;  // 用户取消选择，终止操作
            }
            path = selected;
            // 更新配置中的默认保存目录
            Settings.setSavePath(QFileInfo(selected).path());
        }
        // 确保路径以.wav结尾（不区分大小写）
        if (!path.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive)) {
            path += QStringLiteral(".wav");
        }
        m_outputFile->setText(path);  // 更新输入框显示的最终路径

        // 3. 保存当前配置（下次打开对话框自动应用）
        Settings.setSpeechLanguage(lang);    // 保存语言选择
        Settings.setSpeechVoice(voice);      // 保存声音选择
        Settings.setSpeechSpeed(speed);      // 保存语速选择

        // 4. 打印调试日志（记录用户选择的参数）
        LOG_DEBUG() << "OK clicked, language code:" << lang << "voice:" << voice
                    << "speed:" << speed << "file:" << path;

        // 5. 关闭对话框并返回“成功”状态（后续可触发语音生成逻辑）
        accept();
    });
}

// 【私有方法】：根据语言标识加载对应的声音选项
// 参数：langCode - 语言标识（如“a”=美式英语、“z”=普通话中文）
void SpeechDialog::populateVoices(const QString &langCode)
{
    // 静态常量：所有支持的声音列表（格式“语言前缀_性别_声音名”，如“af_alloy”：a=美式英语、f=女性、alloy=声音名）
    static const QStringList kVoices = {
        QStringLiteral("af_alloy"),    QStringLiteral("af_aoede"),
        QStringLiteral("af_bella"),    QStringLiteral("af_heart"),
        QStringLiteral("af_jessica"),  QStringLiteral("af_kore"),
        QStringLiteral("af_nicole"),   QStringLiteral("af_nova"),
        QStringLiteral("af_river"),    QStringLiteral("af_sarah"),
        QStringLiteral("af_sky"),      QStringLiteral("am_adam"),
        QStringLiteral("am_echo"),     QStringLiteral("am_eric"),
        QStringLiteral("am_fenrir"),   QStringLiteral("am_liam"),
        QStringLiteral("am_michael"),  QStringLiteral("am_onyx"),
        QStringLiteral("am_puck"),     QStringLiteral("am_santa"),
        QStringLiteral("bf_alice"),    QStringLiteral("bf_emma"),
        QStringLiteral("bf_isabella"), QStringLiteral("bf_lily"),
        QStringLiteral("bm_daniel"),   QStringLiteral("bm_fable"),
        QStringLiteral("bm_george"),   QStringLiteral("bm_lewis"),
        QStringLiteral("ef_dora"),     QStringLiteral("em_alex"),
        QStringLiteral("em_santa"),    QStringLiteral("ff_siwis"),
        QStringLiteral("hf_alpha"),    QStringLiteral("hf_beta"),
        QStringLiteral("hm_omega"),    QStringLiteral("hm_psi"),
        QStringLiteral("if_sara"),     QStringLiteral("im_nicola"),
        QStringLiteral("jf_alpha"),    QStringLiteral("jf_gongitsune"),
        QStringLiteral("jf_nezumi"),   QStringLiteral("jf_tebukuro"),
        QStringLiteral("jm_kumo"),     QStringLiteral("pf_dora"),
        QStringLiteral("pm_alex"),     QStringLiteral("pm_santa"),
        QStringLiteral("zf_xiaobei"),  QStringLiteral("zf_xiaoni"),
        QStringLiteral("zf_xiaoxiao"), QStringLiteral("zf_xiaoyi"),
        QStringLiteral("zm_yunjian"),  QStringLiteral("zm_yunxi"),
        QStringLiteral("zm_yunxia"),   QStringLiteral("zm_yunyang"),
    };

    m_voice->clear();  // 清空当前声音下拉框的选项（避免残留旧语言的声音）
    if (langCode.isEmpty()) {  // 若语言标识为空，直接返回（无声音可加载）
        return;
    }

    // 提取语言前缀（取语言标识的第一个字符，如langCode为“a”则前缀为“a”）
    const QString prefix = langCode.left(1);

    // 遍历所有声音，筛选出与当前语言前缀匹配的声音
    for (const auto &v : kVoices) {
        // 若声音的前缀与当前语言前缀一致（如“af_alloy”前缀为“a”，匹配美式英语）
        if (v.startsWith(prefix)) {
            // 找到声音名称中的下划线（分割“语言前缀_性别_声音名”，如“af_alloy”的下划线在索引1处）
            const int underscore = v.indexOf('_');
            // 确保下划线位置有效（避免格式错误的声音名）
            if (underscore > 0 && underscore + 1 < v.size()) {
                // 提取声音名（从下划线后一位开始，如“af_alloy”→“alloy”）
                const QString name = v.mid(underscore + 1);
                // 提取性别标识（声音名前缀的第二个字符，如“af_alloy”→“f”=女性，“am_adam”→“m”=男性）
                const QString gender = v.mid(1, 1).toLower();

                // 根据性别添加对应的图标和格式化的声音名（用户可见）
                if (gender == "m") {
                    // 男性声音：添加“♂️”图标，声音名首字母大写（如“♂️ Adam”）
                    m_voice->addItem(QStringLiteral("♂️ ") + name[0].toUpper() + name.mid(1), v);
                } else if (gender == "f") {
                    // 女性声音：添加“♀️”图标，声音名首字母大写（如“♀️ Alloy”）
                    m_voice->addItem(QStringLiteral("♀️ ") + name[0].toUpper() + name.mid(1), v);
                } else {
                    // 未知性别：仅声音名首字母大写（无图标）
                    m_voice->addItem(name[0].toUpper() + name.mid(1), v);
                }
            }
        }
    }

    // 若筛选后无匹配的声音，添加“(No voices)”选项（提示用户无可用声音）
    if (m_voice->count() == 0) {
        m_voice->addItem(tr("(No voices)"), QString());
    }
}
