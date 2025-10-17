#pragma once
/*****************************************************************************
 * 
 * Copyright 2016 Varol Okan. All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the governing permissions and
 * limitations under the License.
 * 
 ****************************************************************************/

#include <stdint.h>
#include <string>
#include <limits>

#include <QtEndian>

// 【平台兼容性处理】：字节序转换宏定义
// 【说明】：在不同操作系统上提供一致的字节序转换函数
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
// Windows和macOS平台：使用Qt的字节序转换函数
#  define htobe16(x) qToBigEndian(x)     // 主机序转大端序（16位）
#  define htole16(x) qToLittleEndian(x)  // 主机序转小端序（16位）
#  define be16toh(x) qFromBigEndian(x)   // 大端序转主机序（16位）
#  define le16toh(x) qFromLittleEndian(x)// 小端序转主机序（16位）
#  define htobe32(x) qToBigEndian(x)     // 主机序转大端序（32位）
#  define htole32(x) qToLittleEndian(x)  // 主机序转小端序（32位）
#  define be32toh(x) qFromBigEndian(x)   // 大端序转主机序（32位）
#  define le32toh(x) qFromLittleEndian(x)// 小端序转主机序（32位）
#  define htobe64(x) qToBigEndian(x)     // 主机序转大端序（64位）
#  define htole64(x) qtoLittleEndian(x)  // 主机序转小端序（64位）
#  define be64toh(x) qFromBigEndian(x)   // 大端序转主机序（64位）
#  define le64toh(x) qFromLittleEndian(x)// 小端序转主机序（64位）
#elif !defined(__FreeBSD__)
// Linux和其他Unix系统：使用标准endian.h
#  include <endian.h>
#elif defined(__FreeBSD__)
// FreeBSD系统：使用sys/endian.h
#  include <sys/endian.h>
#endif

// 【结构体说明】：音频元数据结构
// 【功能】：存储3D音频和Ambisonic格式的元数据信息
// 【应用】：用于360度视频和空间音频处理
struct AudioMetadata {
  AudioMetadata ( )  {
    ambisonic_order = 1;                          // Ambisonic阶数（默认1阶）
    ambisonic_type  = "periphonic";               // Ambisonic类型
    ambisonic_channel_ordering = "ACN";           // 通道排序方式（ACN标准）
    ambisonic_normalization    = "SN3D";          // 归一化方法（SN3D标准）
    for ( uint32_t t=0; t<4; t++ )
      channel_map[t] = t;                         // 通道映射表初始化
  };
  uint32_t ambisonic_order;                       // Ambisonic阶数（1=一阶，2=二阶等）
  std::string ambisonic_type;                     // Ambisonic类型（"periphonic"等）
  std::string ambisonic_channel_ordering;         // 通道排序（"ACN"=AmbiX格式）
  std::string ambisonic_normalization;            // 归一化方法（"SN3D"=Schmidt半归一化）
  uint32_t channel_map[4];                        // 通道映射表（最多4个通道）
};

// 【命名空间说明】：MPEG-4文件格式常量定义
// 【功能】：定义MP4文件中各种原子（Box）的类型标识符和枚举
namespace constants
{

// 【轨道类型常量】
static const char *TRAK_TYPE_VIDE = "vide";      // 视频轨道类型

// 【叶子原子类型】：包含实际数据的原子
static const char *TAG_STCO = "stco";            // 32位块偏移表（Chunk Offset）
static const char *TAG_CO64 = "co64";            // 64位块偏移表（Chunk Offset 64）
static const char *TAG_FREE = "free";            // 空闲空间原子
static const char *TAG_MDAT = "mdat";            // 媒体数据原子（Media Data）
static const char *TAG_XML  = "xml ";            // XML元数据原子
static const char *TAG_HDLR = "hdlr";            // 处理器引用原子（Handler Reference）
static const char *TAG_FTYP = "ftyp";            // 文件类型原子（File Type）
static const char *TAG_ESDS = "esds";            // 基本流描述原子（Elementary Stream Descriptor）
static const char *TAG_SOUN = "soun";            // 音频轨道处理器类型（Sound）
static const char *TAG_SA3D = "SA3D";            // 空间音频3D元数据原子（Spatial Audio 3D）

// 【容器原子类型】：包含其他原子的容器
static const char *TAG_MOOV = "moov";            // 电影原子（Movie Container）
static const char *TAG_UDTA = "udta";            // 用户数据原子（User Data）
static const char *TAG_META = "meta";            // 元数据原子（Metadata）
static const char *TAG_TRAK = "trak";            // 轨道原子（Track）
static const char *TAG_MDIA = "mdia";            // 媒体原子（Media）
static const char *TAG_MINF = "minf";            // 媒体信息原子（Media Information）
static const char *TAG_STBL = "stbl";            // 样本表原子（Sample Table）
static const char *TAG_STSD = "stsd";            // 样本描述原子（Sample Description）
static const char *TAG_UUID = "uuid";            // 用户自定义原子（Universally Unique Identifier）
static const char *TAG_WAVE = "wave";            // WAVE格式原子

// 【音频样本描述格式】：定义音频编码格式
static const char *TAG_NONE = "NONE";            // 无格式（占位符）
static const char *TAG_RAW_ = "raw ";            // 原始音频数据
static const char *TAG_TWOS = "twos";            // 有符号16位大端序音频
static const char *TAG_SOWT = "sowt";            // 有符号16位小端序音频
static const char *TAG_FL32 = "fl32";            // 32位浮点音频
static const char *TAG_FL64 = "fl64";            // 64位浮点音频
static const char *TAG_IN24 = "in24";            // 24位整数音频
static const char *TAG_IN32 = "in32";            // 32位整数音频
static const char *TAG_ULAW = "ulaw";            // μ-law压缩音频
static const char *TAG_ALAW = "alaw";            // A-law压缩音频
static const char *TAG_LPCM = "lpcm";            // 线性PCM音频
static const char *TAG_MP4A = "mp4a";            // MPEG-4音频

// 【音频样本描述格式数组】：所有支持的音频格式列表
static const char * SOUND_SAMPLE_DESCRIPTIONS[12] = {
    TAG_NONE,    // 0: 无格式
    TAG_RAW_,    // 1: 原始音频
    TAG_TWOS,    // 2: 有符号16位大端序
    TAG_SOWT,    // 3: 有符号16位小端序
    TAG_FL32,    // 4: 32位浮点
    TAG_FL64,    // 5: 64位浮点
    TAG_IN24,    // 6: 24位整数
    TAG_IN32,    // 7: 32位整数
    TAG_ULAW,    // 8: μ-law压缩
    TAG_ALAW,    // 9: A-law压缩
    TAG_LPCM,    // 10: 线性PCM
    TAG_MP4A     // 11: MPEG-4音频
};

// 【容器原子列表】：所有容器类型原子的标识符数组
static const char * CONTAINERS_LIST[20] = {
    TAG_MDIA,    // 媒体原子
    TAG_MINF,    // 媒体信息原子
    TAG_MOOV,    // 电影原子
    TAG_STBL,    // 样本表原子
    TAG_STSD,    // 样本描述原子
    TAG_TRAK,    // 轨道原子
    TAG_UDTA,    // 用户数据原子
    TAG_WAVE,    // WAVE格式原子

    // 以下为音频格式（可能用于特定上下文）
    TAG_NONE,
    TAG_RAW_,
    TAG_TWOS,
    TAG_SOWT,
    TAG_FL32,
    TAG_FL64,
    TAG_IN24,
    TAG_IN32,
    TAG_ULAW,
    TAG_ALAW,
    TAG_LPCM,
    TAG_MP4A
};

  // 【枚举说明】：原子类型分类
  // 【功能】：标识原子在MP4文件结构中的层次和角色
  enum Type {
    Box = 0,          // 基础原子类型（叶子原子）
    Container,        // 容器原子（包含其他原子）
    ContainerLeaf,    // 容器叶子原子（特定类型的容器）
    None              // 未定义类型
  };

};  // End of namespace constants
