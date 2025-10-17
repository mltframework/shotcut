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
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 ****************************************************************************/

// 【文件说明】：MP4文件原子（Box）操作工具头文件
// 【功能】：定义MPEG4文件原子的数据结构和操作接口
#include <stdint.h>
#include <vector>
#include <fstream>

// 【类说明】：MP4文件原子（Box）基类
// 【功能】：表示MP4文件中的基本数据单元，提供加载、保存和操作接口
// 【特性】：支持文件流操作、字节序转换、索引表管理
class Box 
{
public:
    // 【构造函数和析构函数】
    Box ( );
    virtual ~Box ( );
    
    // 【功能】：获取原子类型
    virtual int32_t type ( );

    // 【静态方法】：原子管理和文件操作
    static Box *load  ( std::fstream &, uint32_t, uint32_t );  // 从文件加载原子
    static void clear ( std::vector<Box *> & );                // 清理原子列表

    // 【原子内容操作】
    int content_start ( );                                     // 获取内容起始位置
    virtual void save ( std::fstream &, std::fstream &, int32_t ); // 保存原子到文件
    void set  ( uint8_t *, uint32_t );                         // 设置原子内容数据
    int  size ( );                                             // 计算原子总大小
    const char *name( );                                       // 获取原子名称
    virtual void print_structure ( const char * );             // 打印原子结构（调试）

    // 【数据复制方法】
    void tag_copy   ( std::fstream &, std::fstream &, int32_t );   // 普通数据复制
    void index_copy ( std::fstream &, std::fstream &, Box *, bool, int32_t ); // 索引表复制
    void stco_copy  ( std::fstream &, std::fstream &, Box *, int32_t ); // STCO原子复制
    void co64_copy  ( std::fstream &, std::fstream &, Box *, int32_t ); // CO64原子复制

public:
    // 【静态读取方法】：从文件流读取各种数据类型（大端序）
    static   int8_t readInt8   ( std::fstream &fs );   // 读取8位有符号整数
    static  int16_t readInt16  ( std::fstream &fs );   // 读取16位有符号整数
    static  int32_t readInt32  ( std::fstream &fs );   // 读取32位有符号整数
    static  uint8_t readUint8  ( std::fstream &fs );   // 读取8位无符号整数
    static uint32_t readUint32 ( std::fstream &fs );   // 读取32位无符号整数
    static uint64_t readUint64 ( std::fstream &fs );   // 读取64位无符号整数
    static double   readDouble ( std::fstream &fs );   // 读取双精度浮点数

    // 【静态写入方法】：向文件流写入各种数据类型（转换为大端序）
    static void     writeInt16 ( std::fstream &fs, int16_t  );  // 写入16位有符号整数
    static void     writeInt32 ( std::fstream &fs, int32_t  );  // 写入32位有符号整数
    static void     writeUint8 ( std::fstream &fs, uint8_t  );  // 写入8位无符号整数
    static void     writeUint32( std::fstream &fs, uint32_t );  // 写入32位无符号整数
    static void     writeUint64( std::fstream &fs, uint64_t );  // 写入64位无符号整数

    int32_t  m_iType;  // 【公有成员】：原子类型标识

private:
    // 【私有方法】：从内容数据中读取数值
    uint32_t uint32FromCont ( int32_t &iIDX );  // 从内容数据读取32位无符号整数
    uint64_t uint64FromCont ( int32_t &iIDX );  // 从内容数据读取64位无符号整数
    
    // 【私有方法】：从内存内容复制索引表
    void index_copy_from_contents ( std::fstream &fsOut, Box *pBox, bool bBigMode, int32_t iDelta );

public: 
    // 【原子数据结构成员】
    char      m_name[4];        // 原子名称（4字节ASCII码）
    uint32_t  m_iPosition;      // 原子在文件中的起始位置
    uint32_t  m_iHeaderSize;    // 头部大小（8或16字节）
    uint32_t  m_iContentSize;   // 内容数据大小
    uint8_t  *m_pContents;      // 内容数据指针（动态分配）
};
