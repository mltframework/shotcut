/*
 * Copyright (c) 2021-2023 Meltytech, LLC
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
#include "saveimagedialog.h"

// 引入依赖的业务逻辑头文件
#include "Logger.h"                  // 日志工具类
#include "mltcontroller.h"           // MLT控制器（获取视频生产者、帧时间等）
#include "settings.h"                // 配置类（读取/保存默认路径、导出后缀等）
#include "util.h"                    // 工具类（获取文件对话框选项、检查文件可写性等）

// 引入Qt相关头文件
#include <QDebug>
#include <QtMath>                    // 用于数学计算（如qFloor、qRound）


// 【辅助函数】：从文件类型过滤器文本中提取文件后缀（如从"PNG (*.png)"提取".png"）
// 参数：filterText - 文件类型过滤器文本（如"JPEG (*.jpg *.jpeg)"）
// 返回值：QString - 提取到的文件后缀（如".jpg"），提取失败则返回空字符串
static QString suffixFromFilter(const QString &filterText)
{
    // 分割逻辑：先按"*"分割取第2部分（如"PNG (*.png)"→"*.png)"→取"*.png"），再按")"分割取第1部分，最后按空格分割取第1部分
    QString suffix = filterText.section("*", 1, 1).section(")", 0, 0).section(" ", 0, 0);
    // 若提取的后缀不以"."开头（非合法后缀），则清空
    if (!suffix.startsWith(".")) {
        suffix.clear();
    }
    return suffix;
}


// 【构造函数】：初始化图像保存对话框
// 参数说明：
// - parent：父窗口指针
// - caption：对话框标题
// - image：待保存的图像对象（引用传递，确保操作原始图像）
SaveImageDialog::SaveImageDialog(QWidget *parent, const QString &caption, QImage &image)
    : QFileDialog(parent, caption)  // 继承QFileDialog，初始化父类
    , m_image(image)                // 保存待保存的图像引用
{
    setModal(true);                          // 设置为模态对话框（阻塞父窗口操作）
    setAcceptMode(QFileDialog::AcceptSave);  // 设置对话框模式为"保存"（而非"打开"）
    setFileMode(QFileDialog::AnyFile);       // 设置文件模式为"任意文件"（允许保存新文件）
    setOptions(Util::getFileDialogOptions());// 应用工具类定义的文件对话框选项（如原生风格、支持多选等）
    setDirectory(Settings.savePath());       // 设置默认保存目录（从配置中读取）

    // 1. 设置文件类型过滤器（支持的图像格式：PNG、BMP、JPEG等）
    QString nameFilter = tr("PNG (*.png);;BMP (*.bmp);;JPEG (*.jpg *.jpeg);;PPM (*.ppm);;TIFF "
                            "(*.tif *.tiff);;WebP (*.webp);;All Files (*)");
    setNameFilter(nameFilter);

    // 2. 从配置中读取默认导出后缀，匹配并选中对应的文件类型过滤器
    QStringList nameFilters = nameFilter.split(";;");  // 分割过滤器为列表（如["PNG (*.png)", "BMP (*.bmp)"]）
    QString suffix = Settings.exportFrameSuffix();      // 读取配置的默认后缀（如".png"）
    QString selectedNameFilter = nameFilters[0];        // 默认选中第一个过滤器（PNG）
    // 遍历过滤器列表，找到包含默认后缀的过滤器并选中
    for (const auto &f : nameFilters) {
        if (f.contains(suffix.toLower())) {  // 忽略大小写匹配（如".PNG"和".png"都匹配）
            selectedNameFilter = f;
            break;
        }
    }
    selectNameFilter(selectedNameFilter);  // 选中匹配的过滤器

    // 3. 生成默认文件名（以当前播放器时间为基础）
    // 获取当前MLT生产者的帧时间（mlt_time_clock表示当前时间），格式化为"Shotcut_YYYYMMDD_HH_MM_SS_XXX"
    QString nameSuggestion
        = QStringLiteral("Shotcut_%1").arg(MLT.producer()->frame_time(mlt_time_clock));
    nameSuggestion = nameSuggestion.replace(":", "_");  // 将时间中的":"替换为"_"（避免文件名非法字符）
    nameSuggestion = nameSuggestion.replace(".", "_");  // 将时间中的"."替换为"_"
    nameSuggestion += suffix;  // 拼接默认后缀（如".png"）
    selectFile(nameSuggestion);  // 设置默认文件名

    // 4. 连接信号槽（处理过滤器选择变化和文件选择事件）
    // 非Windows系统：连接过滤器选择变化信号（过滤类型改变时更新文件名后缀）
#if !defined(Q_OS_WIN)
    if (!connect(this, &QFileDialog::filterSelected, this, &SaveImageDialog::onFilterSelected))
        connect(this,
                SIGNAL(filterSelected(const QString &)),
                SLOT(const onFilterSelected(const QString &)));
#endif
    // 连接文件选择信号（用户选择文件后执行保存逻辑）
    if (!connect(this, &QFileDialog::fileSelected, this, &SaveImageDialog::onFileSelected))
        connect(this, SIGNAL(fileSelected(const QString &)), SLOT(onFileSelected(const QString &)));
}


// 【槽函数】：处理文件类型过滤器选择变化事件（更新文件名后缀以匹配选中的过滤器）
// 参数：filter - 选中的文件类型过滤器文本（如"JPEG (*.jpg *.jpeg)"）
void SaveImageDialog::onFilterSelected(const QString &filter)
{
    if (filter.isEmpty()) {  // 过滤器为空则返回
        return;
    }
    QString suffix = suffixFromFilter(filter);  // 从过滤器中提取后缀
    if (suffix.isEmpty()) {
        return;  // 若为"所有文件"过滤器（无后缀），则返回
    }

    // 获取当前选中的文件列表（仅处理第一个文件）
    QStringList files = selectedFiles();
    if (files.size() == 0) {
        return;
    }
    QString filename = files[0];

    // 移除文件名中已有的后缀（若存在）
    if (!QFileInfo(filename).suffix().isEmpty()) {
        filename = filename.section(".", 0, -2);  // 按"."分割，取除最后一部分外的所有内容（如"image.png"→"image"）
    }
    // 拼接新的后缀（如"image" + ".jpg"→"image.jpg"）
    filename += suffix;
    selectFile(filename);  // 更新选中的文件名
}


// 【槽函数】：处理文件选择事件（用户确认选择文件后，执行图像保存逻辑）
// 参数：file - 用户选中的文件路径（如"C:/images/shotcut_20240520_143000.png"）
void SaveImageDialog::onFileSelected(const QString &file)
{
    if (file.isEmpty()) {  // 文件路径为空则返回
        return;
    }
    m_saveFile = file;  // 保存选中的文件路径
    QFileInfo fi(m_saveFile);  // 创建文件信息对象（用于获取文件属性）

    // 1. 若文件名无后缀，自动添加当前过滤器对应的后缀（默认PNG）
    if (fi.suffix().isEmpty()) {
        QString suffix = suffixFromFilter(selectedNameFilter());  // 从当前过滤器提取后缀
        if (suffix.isEmpty()) {
            suffix = ".png";  // 若提取失败（如"所有文件"），默认使用".png"
        }
        m_saveFile += suffix;  // 拼接后缀
        fi = QFileInfo(m_saveFile);  // 更新文件信息对象
    }

    // 2. 检查文件是否可写（若不可写，弹出警告并返回）
    if (Util::warnIfNotWritable(m_saveFile, this, windowTitle()))
        return;

    // 3. 若图像宽高比与MLT配置的显示宽高比（DAR）不匹配，缩放图像为正方形像素
    qreal aspectRatio = (qreal) m_image.width() / m_image.height();  // 计算当前图像的宽高比
    // 比较图像宽高比与MLT配置宽高比（保留3位小数，避免浮点精度问题）
    if (qFloor(aspectRatio * 1000) != qFloor(MLT.profile().dar() * 1000)) {
        // 按MLT宽高比缩放图像：高度不变，计算新宽度，忽略原宽高比，使用平滑缩放
        m_image = m_image.scaled(qRound(m_image.height() * MLT.profile().dar()),
                                 m_image.height(),
                                 Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
    }

    // 4. 保存图像（WebP格式默认质量80，其他格式使用默认质量）
    // 第二个参数为格式（Q_NULLPTR表示自动根据后缀识别），第三个参数为质量（-1表示默认）
    m_image.save(m_saveFile, Q_NULLPTR, (fi.suffix() == "webp") ? 80 : -1);

    // 5. 更新配置：保存当前保存目录和后缀，作为下次默认值
    Settings.setSavePath(fi.path());  // 保存当前目录为默认保存目录
    Settings.setExportFrameSuffix(QStringLiteral(".") + fi.suffix());  // 保存当前后缀为默认导出后缀
}
