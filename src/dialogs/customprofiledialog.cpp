/*
 * Copyright (c) 2013-2025 Meltytech, LLC
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
#include "customprofiledialog.h"
// 包含UI文件生成的头文件（用于访问界面控件）
#include "ui_customprofiledialog.h"
// 引入依赖的业务逻辑头文件
#include "mltcontroller.h"  // MLT多媒体框架控制器（管理视频配置文件）
#include "settings.h"       // 配置类（读取应用数据路径、预览缩放等）
#include "util.h"           // 工具类（提供帧率归一化、公约数计算等功能）
// 引入Qt相关头文件
#include <QDesktopServices>
#include <QDir>                // 目录操作类（用于创建/访问配置文件目录）
#include <QRegularExpression>     // 正则表达式类（用于过滤文件名非法字符）

//【构造函数】：初始化自定义配置对话框
CustomProfileDialog::CustomProfileDialog(QWidget *parent)
    : QDialog(parent)                    //参数：parent - 父窗口指针
    , ui(new Ui::CustomProfileDialog)    // 初始化UI指针，创建界面对象
    , m_fps(0.0)                         // 初始化帧率记录变量（用于检测帧率变化）
{
    ui->setupUi(this);                    // 加载UI布局，初始化界面控件
     // 从当前MLT配置中读取参数，初始化界面控件默认值
    // 分辨率（宽/高）
    ui->widthSpinner->setValue(MLT.profile().width());
    ui->heightSpinner->setValue(MLT.profile().height());
     // 显示宽高比（分子/分母）
    ui->aspectNumSpinner->setValue(MLT.profile().display_aspect_num());
    ui->aspectDenSpinner->setValue(MLT.profile().display_aspect_den());
     // 帧率
    ui->fpsSpinner->setValue(MLT.profile().fps());
    // 扫描模式（逐行/隔行）：0=隔行，1=逐行（与下拉框索引对应）
    ui->scanModeCombo->setCurrentIndex(MLT.profile().progressive());
    // 色彩空间：根据MLT配置值设置下拉框索引（601→0，2020→2，默认→1（709））
    switch (MLT.profile().colorspace()) {
    case 601:
        ui->colorspaceCombo->setCurrentIndex(0);
        break;
    case 2020:
        ui->colorspaceCombo->setCurrentIndex(2);
        break;
    default:
        ui->colorspaceCombo->setCurrentIndex(1);
        break;
    }
}
// 【析构函数】：释放UI资源
CustomProfileDialog::~CustomProfileDialog()
{
    delete ui;        // 释放UI对象占用的内存
}

// 【公共方法】：获取合法的配置文件名
// 返回值：QString - 过滤非法字符后的文件名
QString CustomProfileDialog::profileName() const
{
    // 1. 获取用户输入的文件名
    QString filename = ui->nameEdit->text();
    // 2. 定义Windows文件名非法字符正则表达式（\ / : * ? " < > |）
    static QRegularExpression re("[" + QRegularExpression::escape("\\/:*?\"<>|") + "]");
    // 3. 将非法字符替换为下划线，返回合法文件名
    filename = filename.replace(re, QStringLiteral("_"));
    return filename;
}

// 【槽函数】：确认按钮（OK）点击事件（保存配置）
void CustomProfileDialog::on_buttonBox_accepted()
{
    // 1. 更新MLT配置文件参数（将界面输入值同步到MLT）
    MLT.profile().set_explicit(1);
    // 分辨率
    MLT.profile().set_width(ui->widthSpinner->value());
    MLT.profile().set_height(ui->heightSpinner->value());
    // 显示宽高比（分子/分母）
    MLT.profile().set_display_aspect(ui->aspectNumSpinner->value(), ui->aspectDenSpinner->value());
    // 2. 计算并设置采样宽高比（SAR）
    // 公式：SAR = (显示宽高比分子 × 高度) : (显示宽高比分母 × 宽度)
    QSize sar(ui->aspectNumSpinner->value() * ui->heightSpinner->value(),
              ui->aspectDenSpinner->value() * ui->widthSpinner->value());
     // 计算SAR分子和分母的最大公约数（用于约分，得到最简比）
    auto gcd = Util::greatestCommonDivisor(sar.width(), sar.height());
    MLT.profile().set_sample_aspect(sar.width() / gcd, sar.height() / gcd);
    // 3. 归一化帧率并设置（将浮点数帧率转换为分数形式，如29.97→30000/1001）
    int numerator, denominator;
    Util::normalizeFrameRate(ui->fpsSpinner->value(), numerator, denominator);
    MLT.profile().set_frame_rate(numerator, denominator);
    // 4. 设置扫描模式（逐行/隔行）
    MLT.profile().set_progressive(ui->scanModeCombo->currentIndex());
     // 5. 设置色彩空间（根据下拉框索引对应值）
    switch (ui->colorspaceCombo->currentIndex()) {
    case 0:
        MLT.profile().set_colorspace(601);
        break;
    case 2:
        MLT.profile().set_colorspace(2020);
        break;
    default:
        MLT.profile().set_colorspace(709);
        break;
    }
     // 6. 更新预览配置（使新配置生效）
    MLT.updatePreviewProfile();
    MLT.setPreviewScale(Settings.playerPreviewScale());
    // 7. 保存配置文件到本地（如果用户输入了文件名）
    // Save it to a file
    if (!ui->nameEdit->text().isEmpty()) {
        // 7.1 获取应用数据存储目录（从配置中读取）
        QDir dir(Settings.appDataLocation());
        QString subdir("profiles");
        // 7.2 创建目录（如果不存在）
        if (!dir.exists())
            dir.mkpath(dir.path());
        // 进入"profiles"子目录，若不存在则创建
        if (!dir.cd(subdir)) {
            if (dir.mkdir(subdir))
                dir.cd(subdir);
        }
        // 7.3 构建配置文件内容（将MLT配置参数存入Properties）
        Mlt::Properties p;
        p.set("width", MLT.profile().width());
        p.set("height", MLT.profile().height());
        p.set("sample_aspect_num", MLT.profile().sample_aspect_num());
        p.set("sample_aspect_den", MLT.profile().sample_aspect_den());
        p.set("display_aspect_num", MLT.profile().display_aspect_num());
        p.set("display_aspect_den", MLT.profile().display_aspect_den());
        p.set("progressive", MLT.profile().progressive());
        p.set("colorspace", MLT.profile().colorspace());
        p.set("frame_rate_num", MLT.profile().frame_rate_num());
        p.set("frame_rate_den", MLT.profile().frame_rate_den());
        // 7.4 保存配置文件（路径：应用数据目录/profiles/合法文件名）
        p.save(dir.filePath(profileName()).toUtf8().constData());
    }
}
// 【槽函数】：宽度输入框编辑完成事件
void CustomProfileDialog::on_widthSpinner_editingFinished()
{
     // 将宽度值调整为"特定倍数"（如偶数，由Util::coerceMultiple实现），确保符合视频标准
    ui->widthSpinner->setValue(Util::coerceMultiple(ui->widthSpinner->value()));
}
// 【槽函数】：高度输入框编辑完成事件
void CustomProfileDialog::on_heightSpinner_editingFinished()
{
    ui->heightSpinner->setValue(Util::coerceMultiple(ui->heightSpinner->value()));
}
// 【槽函数】：帧率输入框编辑完成事件
void CustomProfileDialog::on_fpsSpinner_editingFinished()
{
    // 仅当帧率发生变化时执行（避免重复触发）
    if (ui->fpsSpinner->value() != m_fps) {
        const QString caption(tr("Video Mode Frames/sec"));
        // 根据输入的帧率，显示对应的"精确帧率"对话框（如23.98→24000/1001）
        if (ui->fpsSpinner->value() == 23.98 || ui->fpsSpinner->value() == 23.976) {
            Util::showFrameRateDialog(caption, 24000, ui->fpsSpinner, this);
        } else if (ui->fpsSpinner->value() == 29.97) {
            Util::showFrameRateDialog(caption, 30000, ui->fpsSpinner, this);
        } else if (ui->fpsSpinner->value() == 47.95) {
            Util::showFrameRateDialog(caption, 48000, ui->fpsSpinner, this);
        } else if (ui->fpsSpinner->value() == 59.94) {
            Util::showFrameRateDialog(caption, 60000, ui->fpsSpinner, this);
        }
        m_fps = ui->fpsSpinner->value();
    }
}
// 【槽函数】：帧率下拉框文本激活事件
void CustomProfileDialog::on_fpsComboBox_textActivated(const QString &arg1)
{
    if (arg1.isEmpty())
        return;
     // 将下拉框选择的文本（如"24"）转换为浮点数，设置到帧率输入框
    ui->fpsSpinner->setValue(arg1.toDouble());
}
// 【槽函数】：分辨率下拉框文本激活事件
void CustomProfileDialog::on_resolutionComboBox_textActivated(const QString &arg1)
{
    if (arg1.isEmpty())
        return;
   // 拆分分辨率文本（如"1920 x 1080"→["1920", "x", "1080"]）
    auto parts = arg1.split(' ');
    // 将宽、高分别设置到对应的输入框
    ui->widthSpinner->setValue(parts[0].toInt());
    ui->heightSpinner->setValue(parts[2].toInt());
}
// 【槽函数】：宽高比下拉框文本激活事件
void CustomProfileDialog::on_aspectRatioComboBox_textActivated(const QString &arg1)
{
    if (arg1.isEmpty())
        return;
     // 拆分宽高比文本（如"16:9 (1.78)"→["16:9", "(1.78)"]→["16", "9"]）
    auto parts = arg1.split(' ')[0].split(':');
    // 将宽高比分子、分母设置到对应的输入框
    ui->aspectNumSpinner->setValue(parts[0].toInt());
    ui->aspectDenSpinner->setValue(parts[1].toInt());
}
