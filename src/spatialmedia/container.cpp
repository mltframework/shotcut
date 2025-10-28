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

#include <iostream>
#include <assert.h>
#include <string.h>

#include "constants.h"
#include "container.h"
#include "sa3d.h"

// 【构造函数】：初始化容器原子
// 【参数】：iPadding - 填充大小（用于特定格式的样本描述）
Container::Container ( uint32_t iPadding )
   : Box ( )  // 调用基类构造函数
{
  m_iType = constants::Container;  // 设置为容器类型
  m_iPadding = iPadding;           // 设置填充大小
}

// 【析构函数】
Container::~Container ( )
{
  // 注意：基类析构函数会自动清理m_listContents中的Box对象
}

// 【核心功能】：从文件流加载容器原子
// 【说明】：容器原子可以包含其他原子，形成层次结构
Box *Container::load ( std::fstream &fs, uint32_t iPos, uint32_t iEnd )
{
  fs.seekg ( iPos );                    // 定位到指定位置
  uint32_t iHeaderSize = 8;             // 默认头部大小8字节
  uint32_t iSize = readUint32 ( fs );   // 读取原子大小
  char name[4];
  fs.read ( name, 4 );                  // 读取原子名称

  // 检查是否为容器原子：在容器列表中查找当前原子名称
  int32_t iArrSize = (int32_t)(sizeof(constants::CONTAINERS_LIST)/sizeof(constants::CONTAINERS_LIST[0]));
  bool bIsBox = true;  // 默认为基础原子（非容器）
  
  for ( auto t=0; t<iArrSize; t++ )  {
    if ( memcmp ( name, constants::CONTAINERS_LIST[t], 4 ) == 0 )  {
      bIsBox = false;  // 找到匹配，标记为容器原子
      break;
    }
  }

  // 特殊处理：MP4A解压缩器设置（wave -> mp4a转换）
  if ( memcmp ( name, constants::TAG_MP4A, 4 ) == 0 && iSize == 12 ) 
    bIsBox = true;  // 特定条件下的MP4A原子视为基础原子

  // 如果是基础原子，委托给相应的加载器
  if ( bIsBox )  {
    if ( memcmp ( name, constants::TAG_SA3D, 4 ) == 0 )
      return SA3DBox::load ( fs, iPos, iEnd );  // SA3D空间音频原子
    return Box::load ( fs, iPos, iEnd );        // 普通基础原子
  }

  // 处理扩展大小格式（当size=1时）
  if( iSize == 1 )  {
    iSize = (uint32_t)readUint64 ( fs );  // 读取64位实际大小
    iHeaderSize = 16;                     // 扩展头部大小16字节
  }

  // 大小有效性检查
  if ( iSize < 8 )  {
    std::cerr << "Error, invalid size " << iSize << " in " << name << " at " << iPos << std::endl;
    return NULL;
  }

  // 边界检查
  if ( iPos + iSize > iEnd )  {
    std::cerr << "Error: Container box size exceeds bounds." << std::endl;
    return NULL;
  }

  // 计算填充大小：针对特定原子类型设置不同的填充值
  uint32_t iPadding = 0;
  
  // STSD（样本描述）原子需要特殊填充处理
  if ( memcmp ( name, constants::TAG_STSD, 4 ) == 0 )
    iPadding = 8;  // STSD原子基础填充

  // 检查音频样本描述格式，设置相应的填充大小
  uint32_t iCurrentPos = 0;
  int16_t  iSampleDescVersion = 0;
  iArrSize = (int32_t)(sizeof(constants::SOUND_SAMPLE_DESCRIPTIONS)/sizeof(constants::SOUND_SAMPLE_DESCRIPTIONS[0]));
  
  for ( auto t=0; t<iArrSize; t++ )  {
    if ( memcmp ( name, constants::SOUND_SAMPLE_DESCRIPTIONS[t], 4 ) == 0 )  {
      // 读取样本描述版本号
      iCurrentPos = fs.tellg ( );
      fs.seekg ( iCurrentPos + 8 );              // 跳过前8字节
      fs.read  ( (char *)&iSampleDescVersion, 2 ); // 读取版本号（2字节）
      iSampleDescVersion = be16toh ( iSampleDescVersion );  // 转换为主机序
      fs.seekg ( iCurrentPos );                  // 恢复文件位置

      // 根据版本号设置不同的填充大小
      switch ( iSampleDescVersion )  {
      case 0:
        iPadding = 28;      // 版本0：28字节填充
        break;
      case 1:
        iPadding = 28 + 16; // 版本1：44字节填充（28+16）
        break;
      case 2:
        iPadding = 64;      // 版本2：64字节填充
        break;
      default:
        std::cerr << "Unsupported sample description version:" << iSampleDescVersion << std::endl;
        break;
      }
    }
  }

  // 创建新的容器原子对象
  Container *pNewBox = new Container ( iPadding );
  memcpy ( pNewBox->m_name, name, 4 );
  pNewBox->m_iPosition    = iPos;
  pNewBox->m_iHeaderSize  = iHeaderSize;
  pNewBox->m_iContentSize = iSize - iHeaderSize;
  
  // 递归加载容器内的所有子原子
  pNewBox->m_listContents = load_multiple ( fs, iPos + iHeaderSize + iPadding, iPos + iSize );

  // 检查是否成功加载了子原子
  if ( pNewBox->m_listContents.empty ( ) )  {
    delete pNewBox;
    return NULL;  // 加载失败
  }

  return pNewBox;
}

// 【功能】：从文件流加载多个原子
// 【说明】：在指定范围内连续加载原子，直到达到结束位置
std::vector<Box *> Container::load_multiple ( std::fstream &fs, uint32_t iPos, uint32_t iEnd )
{
  std::vector<Box *> list, empty;
  
  // 循环加载直到达到结束位置
  while ( iPos < iEnd )  {
    Box *pBox = load ( fs, iPos, iEnd );  // 加载单个原子
    if ( ! pBox )  {
      std::cerr << "Error, failed to load box." << std::endl;
      clear ( list );  // 清理已加载的原子
      return empty;    // 返回空列表
    }
    list.push_back ( pBox );                    // 添加到列表
    iPos = pBox->m_iPosition + pBox->size ( );  // 更新位置到下一个原子
  }
  return list;
}

// 【功能】：重新计算容器大小
// 【说明】：递归更新容器及其所有子原子的大小
void Container::resize ( )
{
  m_iContentSize = m_iPadding;  // 重置内容大小（从填充开始）
  
  // 遍历所有子原子，累加它们的大小
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pBox = *it++;
    if ( pBox->type ( ) == constants::Container )  {
      Container *p = (Container *)pBox;
      p->resize ( );  // 递归调整子容器大小
    }
    m_iContentSize += pBox->size ( );  // 累加子原子大小
  }
}

// 【功能】：打印容器结构（树形显示）
// 【说明】：递归打印容器及其所有子原子的层次结构
void Container::print_structure ( const char *pIndent )
{
  // 打印当前容器的基本信息
  uint32_t iSize1 = m_iHeaderSize;
  uint32_t iSize2 = m_iContentSize;
  std::cout <<  "{" << pIndent << "} {" << name ( ) << "} [{" << iSize1 << "}, {" << iSize2 << "}]" << std::endl;

  // 准备缩进字符串用于子原子显示
  int32_t iCount = m_listContents.size ( );
  std::string strIndent = pIndent;
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  
  // 递归打印所有子原子
  while ( it != m_listContents.end ( ) )  {
    Box *pBox = *it++;
    if ( ! pBox )
      continue;
      
    // 更新缩进字符串（树形结构显示）
    strIndent.replace ( strIndent.begin ( ), strIndent.end ( ), "├", "│" );
    strIndent.replace ( strIndent.begin ( ), strIndent.end ( ), "└", " " );
    strIndent.replace ( strIndent.begin ( ), strIndent.end ( ), "─", " " );

    // 根据是否是最后一个子原子选择不同的连接符
    if ( --iCount <= 0 )
      strIndent += " └──";  // 最后一个子原子
    else
      strIndent += " ├──";  // 中间子原子
      
    pBox->print_structure ( strIndent.c_str ( ) );  // 递归打印子原子
  }
}

// 【功能】：从容器中移除指定名称的原子
// 【说明】：递归移除所有匹配名称的原子（包括子容器中的）
void Container::remove ( const char *pName )
{
  std::vector<Box *> list;
  m_iContentSize = 0;  // 重置内容大小
  
  // 遍历所有子原子
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pBox = *it++;
    if ( ! pBox )
      continue;
      
    // 检查原子名称是否匹配要移除的名称
    if ( memcmp ( pName, pBox->m_name, 4 ) != 0 )  {
      list.push_back ( pBox );  // 不匹配，保留原子
      
      // 如果是容器原子，递归移除其中的匹配原子
      if ( pBox->type ( ) == constants::Container )  {
        Container *p = (Container *)pBox;
        p->remove ( pName );
      }
      m_iContentSize += pBox->size ( );  // 累加保留原子的大小
    }
    else  {
      delete pBox;  // 匹配，删除原子
    }
  }
  m_listContents = list;  // 更新子原子列表
}

// 【功能】：向容器中添加原子
// 【说明】：如果存在同类型的容器，会尝试合并而不是直接添加
bool Container::add ( Box *pElement )
{
  // 检查是否已存在同类型的原子
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pBox = *it++;
    if ( memcmp ( pElement->m_name, pBox->m_name, 4 ) == 0 )  {
      // 存在同名原子，尝试合并
      if ( pBox->type ( ) == constants::ContainerLeaf )  {
        Container *p = (Container *)pBox;
        return p->merge ( pElement );  // 合并到现有容器
      }
      std::cerr << "Error, cannot merge leafs." << std::endl;
      return false;  // 不能合并叶子原子
    }
  }
  
  // 不存在同名原子，直接添加
  m_listContents.push_back ( pElement );
  return true;
}

// 【功能】：合并两个容器
// 【说明】：将源容器的所有子原子合并到当前容器中
bool Container::merge ( Box *pElem )
{
  assert ( pElem->type ( ) == constants::Container );  // 确保是容器类型
  Container *pElement = (Container *)pElem;
  
  // 验证容器名称匹配
  int iRet = memcmp ( m_name, pElement->m_name, 4 );
  assert ( iRet == 0 );
  
  // 遍历源容器的所有子原子，添加到当前容器
  std::vector<Box *>::iterator it = pElement->m_listContents.begin ( );
  while ( it != pElement->m_listContents.end ( ) )  {
    Box *pSubElement = *it++;
    if ( ! add ( pSubElement ) )  // 添加子原子
      return false;
  }
  return true;
}

// 【功能】：保存容器到输出文件流
// 【说明】：递归保存容器及其所有子原子
void Container::save ( std::fstream &fsIn, std::fstream &fsOut, int32_t iDelta )
{
  // 写入容器头部
  if ( m_iHeaderSize == 16 )  {
    // 扩展大小格式
    writeUint32 ( fsOut, 1 );           // 写入扩展标记
    fsOut.write ( m_name, 4 );          // 写入原子名称
    writeUint64 ( fsOut, size ( ) );    // 写入扩展大小
  }
  else if ( m_iHeaderSize == 8 )  {
    // 标准大小格式
    writeUint32 ( fsOut, size ( ) );    // 写入标准大小
    fsOut.write ( m_name, 4 );          // 写入原子名称
  }
  
  // 写入填充数据（如果存在）
  if ( m_iPadding > 0 )  {
    fsIn.seekg ( content_start ( ) );
    Box::tag_copy ( fsIn, fsOut, m_iPadding );  // 复制填充数据
  }

  // 递归保存所有子原子
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pElement = *it++;
    if ( ! pElement )
      continue;
    pElement->save ( fsIn, fsOut, iDelta );  // 递归保存子原子
  }
}
