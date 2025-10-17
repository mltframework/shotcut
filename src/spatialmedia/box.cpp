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
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 ****************************************************************************/

// 【文件说明】：MP4文件原子（Box）操作工具
// 【功能】：加载MPEG4文件并操作文件结构中的原子（基本数据单元）
#include <string.h>
#include <iostream>

#include "constants.h"
#include "box.h"

// 【构造函数】：初始化Box对象
// 【说明】：设置原子名称、类型、位置和大小等基础属性
Box::Box ( )
{
  memset ( (char *)m_name, ' ', sizeof ( m_name ) );  // 原子名称初始化为空格
  m_iType         = constants::Box;  // 设置为基础Box类型
  m_iPosition     = 0;               // 在文件中的起始位置
  m_iHeaderSize   = 0;               // 头部大小（8或16字节）
  m_iContentSize  = 0;               // 内容数据大小
  m_pContents     = NULL;            // 内容数据指针
}

// 【析构函数】：清理Box对象资源
Box::~Box ( )
{
  delete m_pContents;           // 释放内容数据内存
  m_pContents = NULL;           // 置空指针
  m_iContentSize = m_iHeaderSize = m_iPosition = 0;  // 重置大小和位置
}

// 【功能】：从文件流读取双精度浮点数（8字节）
// 【说明】：处理大端序到主机字节序的转换
double Box::readDouble ( std::fstream &fs )
{
  union {
    double  fVal;   // 浮点数值
    int64_t iVal;   // 整数值（用于字节序转换）
    char bytes[8];  // 字节数组
  } buf;
  fs.read ( buf.bytes, 8 );           // 读取8字节
  buf.iVal = be64toh ( buf.iVal );    // 大端序转主机序
  return buf.fVal;                    // 返回浮点数值
}

// 【功能】：从文件流读取8位无符号整数
uint8_t Box::readUint8 ( std::fstream &fs )
{
  union {
    uint8_t iVal;
    char bytes[1];
  } buf;
  fs.read ( buf.bytes, 1 );
  return buf.iVal;  // 单字节无需字节序转换
}

// 【功能】：从文件流读取8位有符号整数
int8_t Box::readInt8 ( std::fstream &fs )
{
  union {
    int8_t iVal;
    char bytes[1];
  } buf;
  fs.read ( buf.bytes, 1 );
  return buf.iVal;  // 单字节无需字节序转换
}

// 【功能】：从文件流读取16位有符号整数
int16_t Box::readInt16 ( std::fstream &fs )
{
  union {
    int16_t iVal;
    char bytes[2];
  } buf;
  fs.read ( buf.bytes, 2 );
  return be16toh ( buf.iVal );  // 大端序转主机序
}

// 【功能】：从文件流读取32位有符号整数
int32_t Box::readInt32 ( std::fstream &fs )
{
  union {
    int32_t iVal;
    char bytes[4];
  } buf;
  fs.read ( buf.bytes, 4 );
  return be32toh ( buf.iVal );  // 大端序转主机序
}

// 【功能】：从文件流读取32位无符号整数
uint32_t Box::readUint32 ( std::fstream &fs )
{
  union {
    uint32_t iVal;
    char bytes[4];
  } buf;
  fs.read ( buf.bytes, 4 );
  return be32toh ( buf.iVal );  // 大端序转主机序
}

// 【功能】：从文件流读取64位无符号整数
uint64_t Box::readUint64 ( std::fstream &fs )
{
  union {
    uint64_t iVal;
    char bytes[8];
  } buf;
  fs.read ( buf.bytes, 8 );
  return be64toh ( buf.iVal );  // 大端序转主机序
}

// 【功能】：向文件流写入8位无符号整数
void Box::writeUint8 ( std::fstream &fs, uint8_t iVal )
{
  union {
    uint8_t iVal;
    char bytes[1];
  } buf;
  buf.iVal = iVal;  // 单字节无需字节序转换
  fs.write ( (char *)buf.bytes, 1 );
}

// 【功能】：向文件流写入16位有符号整数
void Box::writeInt16 ( std::fstream &fs, int16_t iVal )
{
  union {
    int16_t iVal;
    char bytes[2];
  } buf;
  buf.iVal = htobe16 ( iVal );  // 主机序转大端序
  fs.write ( (char *)buf.bytes, 2 );
}

// 【功能】：向文件流写入32位有符号整数
void Box::writeInt32 ( std::fstream &fs, int32_t iVal )
{
  union {
    int32_t iVal;
    char bytes[4];
  } buf;
  buf.iVal = htobe32 ( iVal );  // 主机序转大端序
  fs.write ( (char *)buf.bytes, 4 );
}

// 【功能】：向文件流写入32位无符号整数
void Box::writeUint32 ( std::fstream &fs, uint32_t iVal )
{
  union {
    uint32_t iVal;
    char bytes[4];
  } buf;
  buf.iVal = htobe32 ( iVal );  // 主机序转大端序
  fs.write ( (char *)buf.bytes, 4 );
}

// 【功能】：向文件流写入64位无符号整数
void Box::writeUint64 ( std::fstream &fs, uint64_t iVal )
{
  union {
    uint64_t iVal;
    char bytes[8];
  } buf;
  buf.iVal = htobe64 ( iVal );  // 主机序转大端序
  fs.write ( (char *)buf.bytes, 8 );
}

// 【功能】：获取原子名称（以null结尾的字符串）
const char *Box::name ( )
{
  static char name[5];
  memcpy ( name, m_name, 4 );  // 复制4字节名称
  name[4] = 0;                 // 添加字符串结束符
  return name;
}

// 【核心功能】：从文件流加载Box对象
// 【参数】：fs - 文件流，iPos - 起始位置，iEnd - 结束位置
// 【返回值】：成功加载的Box对象指针，失败返回NULL
Box *Box::load ( std::fstream &fs, uint32_t iPos, uint32_t iEnd )
{
  // 加载位于MP4文件中指定位置的原子
  uint32_t iHeaderSize = 0;
  uint64_t iSize = 0LL;
  char name[4];

  fs.seekg ( iPos );                    // 定位到指定位置
  iHeaderSize = 8;                      // 默认头部大小8字节
  iSize = (uint64_t)readUint32 ( fs );  // 读取原子大小
  fs.read  ( name, 4 );                 // 读取原子名称

  // 处理扩展大小（当size=1时，实际大小在后续8字节）
  if ( iSize == 1 )  {
    iSize = readUint64 ( fs );
    iHeaderSize = 16;                   // 扩展头部大小16字节
  }
  
  // 大小有效性检查
  if ( iSize < 8 )  { 
    std::cerr << "Error, invalid size " << iSize << " in " << name << " at " << iPos << std::endl;
    return NULL;
  }

  // 边界检查
  if ( iPos + iSize > iEnd )  {
    std::cerr << "Error: Leaf box size exceeds bounds." << std::endl;
    return NULL;
  }
  
  // 创建新的Box对象并初始化
  Box *pNewBox = new Box ( );
  memcpy ( pNewBox->m_name, name, sizeof ( name ) );
  pNewBox->m_iPosition    = iPos;
  pNewBox->m_iHeaderSize  = iHeaderSize;
  pNewBox->m_iContentSize = iSize - iHeaderSize;  // 计算内容大小
  pNewBox->m_pContents = NULL;
  return pNewBox;
}

// 【功能】：获取原子类型
int32_t Box::type ( )
{
  return m_iType;
}

// 【功能】：清理Box对象列表
void Box::clear ( std::vector<Box *> &list )
{
  std::vector<Box *>::iterator it = list.begin ( );
  while (it != list.end ( ) )  {
    delete *it++;  // 删除每个Box对象
  }
  list.clear ( );  // 清空列表
}

// 【功能】：获取内容数据在文件中的起始位置
int32_t Box::content_start ( )
{
  return m_iPosition + m_iHeaderSize;
}

// 【功能】：保存Box内容到输出文件流
// 【参数】：fsIn - 输入文件流，fsOut - 输出文件流，iDelta - 索引更新量
void Box::save ( std::fstream &fsIn, std::fstream &fsOut, int32_t iDelta )
{
  // 保存Box内容，优先使用设置的内容数据
  
  // 写入头部信息
  if ( m_iHeaderSize == 16 )  {
    // 扩展大小格式
    uint64_t iBigSize = size ( );
    writeUint32 ( fsOut, 1 );        // 写入扩展标记
    fsOut.write ( m_name,4 );        // 写入原子名称
    writeUint64 ( fsOut, iBigSize ); // 写入扩展大小
  }
  else if ( m_iHeaderSize == 8 )  {
    // 标准大小格式
    writeUint32 ( fsOut, size ( ) ); // 写入标准大小
    fsOut.write ( m_name,  4 );      // 写入原子名称
  }

  // 定位到内容起始位置
  if ( content_start ( ) )
    fsIn.seekg ( content_start ( ) );

  // 根据原子类型选择不同的复制策略
  if ( memcmp ( m_name, constants::TAG_STCO, 4 ) == 0 )
    stco_copy ( fsIn, fsOut, this, iDelta );    // STCO块复制
  else if ( memcmp ( m_name, constants::TAG_CO64, 4 ) == 0 )
    co64_copy ( fsIn, fsOut, this, iDelta );    // CO64块复制
  else if ( m_pContents )
    fsOut.write ( (char *)m_pContents, m_iContentSize );  // 使用内存内容
  else
    tag_copy ( fsIn, fsOut, m_iContentSize );   // 普通数据复制
}

// 【功能】：设置/覆盖Box内容数据
void Box::set ( uint8_t *pNewContents, uint32_t iSize )
{
  m_pContents = pNewContents;   // 设置新内容指针
  m_iContentSize = iSize;       // 更新内容大小
}

// 【功能】：计算Box总大小（头部 + 内容）
int32_t Box::size ( )
{
  return m_iHeaderSize + m_iContentSize;
}

// 【功能】：打印Box结构信息（用于调试）
void Box::print_structure ( const char *pIndent )
{
  std::cout << "{" << pIndent << "}" << "{" << name ( ) << "} ";
  std::cout << "[{" << m_iHeaderSize << "}, {" << m_iContentSize << "}]" << std::endl; 
}

// 【功能】：从输入流复制数据到输出流
// 【说明】：为避免32位系统内存限制，使用64MB分块复制
void Box::tag_copy ( std::fstream &fsIn, std::fstream &fsOut, int32_t iSize )
{
  int32_t block_size = 64 * 1024 * 1024;  // 64MB分块大小
  m_pContents = new uint8_t[block_size + 1];
  
  // 分块复制数据
  while ( iSize > block_size )  {
    fsIn.read   ( (char *)m_pContents, block_size );
    fsOut.write ( (char *)m_pContents, block_size );
    iSize -= block_size;
  }
  
  // 复制剩余数据
  fsIn.read   ( (char *)m_pContents, iSize );
  fsOut.write ( (char *)m_pContents, iSize );
}

// 【功能】：复制和更新索引表（用于STCO/CO64原子）
// 【参数】：bBigMode - 大端序64位模式，iDelta - 索引偏移量
void Box::index_copy ( std::fstream &fsIn, std::fstream &fsOut, Box *pBox, bool bBigMode, int32_t iDelta )
{
  std::fstream &fs = fsIn;
  
  // 选择数据源：文件流或内存内容
  if ( ! pBox->m_pContents )
    fs.seekg ( pBox->content_start ( ) );
  else  {
    return index_copy_from_contents ( fsOut, pBox, bBigMode, iDelta );
  }

  // 读取索引表头部
  uint32_t iHeader = readUint32 ( fs );
  uint32_t iValues = readUint32 ( fs );
 
  // 写入索引表头部
  writeUint32 ( fsOut, iHeader );
  writeUint32 ( fsOut, iValues );
  
  // 根据模式复制和更新索引值
  if ( bBigMode )  {
   for ( auto i = 0U; i<iValues; i++ )  {
      uint64_t iVal = readUint64 ( fsIn ) + iDelta;  // 更新索引值
      writeUint64 ( fsOut, iVal );
    }
  }
  else  {
    for ( auto i = 0U; i<iValues; i++ )  {
      uint32_t iVal = readUint32 ( fsIn ) + iDelta;  // 更新索引值
      writeUint32 ( fsOut, iVal );
    }
  }
}

// 【功能】：从内容数据中读取32位无符号整数
uint32_t Box::uint32FromCont ( int32_t &iIDX )
{
  union {
    uint32_t iVal;
    char bytes[4];
  } buf;
  memcpy ( buf.bytes, (char *)&m_pContents[iIDX], 4 );
  iIDX += 4;
  return be32toh ( buf.iVal );  // 大端序转主机序
}

// 【功能】：从内容数据中读取64位无符号整数
uint64_t Box::uint64FromCont ( int32_t &iIDX )
{
  union {
    uint64_t iVal;
    char bytes[8];
  } buf;
  memcpy ( buf.bytes, (char *)&m_pContents[iIDX], 8 );
  iIDX += 8;
  return be64toh ( buf.iVal );  // 大端序转主机序
}

// 【功能】：从内存内容复制和更新索引表
void Box::index_copy_from_contents ( std::fstream &fsOut, Box *pBox, bool bBigMode, int32_t iDelta )
{
  (void) pBox; // 未使用参数
  int32_t iIDX = 0;

  // 从内容数据读取头部信息
  uint32_t iHeader = uint32FromCont ( iIDX );
  uint32_t iValues = uint32FromCont ( iIDX );
  
  // 写入头部信息
  writeUint32 ( fsOut, iHeader );
  writeUint32 ( fsOut, iValues );
  
  // 根据模式复制和更新索引值
  if ( bBigMode )  {
   for ( auto i = 0U; i<iValues; i++ )  {
      uint64_t iVal = uint64FromCont ( iIDX ) + iDelta;
      writeUint64 ( fsOut, iVal );
    }
  }
  else  {
    for ( auto i = 0U; i<iValues; i++ )  {
      uint32_t iVal = uint32FromCont ( iIDX ) + iDelta;
      writeUint32 ( fsOut, iVal );
    }
  }
}

// 【功能】：STCO原子复制（32位块偏移）
void Box::stco_copy  ( std::fstream &fsIn, std::fstream &fsOut, Box *pBox, int32_t iDelta )
{
  index_copy ( fsIn, fsOut, pBox, false, iDelta );  // 32位模式
}

// 【功能】：CO64原子复制（64位块偏移）
void Box::co64_copy  ( std::fstream &fsIn, std::fstream &fsOut, Box *pBox, int32_t iDelta )
{
  index_copy ( fsIn, fsOut, pBox, true, iDelta );   // 64位模式
}
