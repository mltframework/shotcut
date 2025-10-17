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

// 【文件说明】：MPEG处理类头文件
// 【功能】：定义MPEG文件加载和原子操作的容器类接口
// 【特性】：支持容器原子的层次结构管理和递归操作
#include <vector>

#include "box.h"

// 【类说明】：容器原子类（继承自Box基类）
// 【功能】：表示MP4文件中可以包含其他原子的容器结构
// 【特性】：支持子原子管理、递归操作、动态调整大小
class Container : public Box
{
public:
    // 【构造函数】：初始化容器原子
    // 【参数】：iPadding - 填充大小（用于特定格式的样本描述）
    Container ( uint32_t iPadding=0 );
    
    // 【析构函数】：清理容器资源
    virtual ~Container ( );

    // 【静态方法】：容器加载操作
    // 【功能】：从文件流加载容器原子（支持递归加载子原子）
    static Box *load ( std::fstream &, uint32_t iPos, uint32_t iEnd );
    
    // 【功能】：从文件流连续加载多个原子
    static std::vector<Box *>load_multiple ( std::fstream &, uint32_t iPos, uint32_t iEnd );

    // 【容器管理方法】
    
    // 【功能】：重新计算容器大小（递归更新所有子原子）
    void resize ( );
    
    // 【功能】：打印容器结构（树形显示层次关系）
    virtual void print_structure ( const char * );
    
    // 【功能】：从容器中移除指定名称的原子（递归操作）
    void remove ( const char * );
    
    // 【功能】：向容器中添加原子（支持合并同名容器）
    bool add    ( Box * );
    
    // 【功能】：合并两个容器（将源容器的子原子合并到当前容器）
    bool merge  ( Box * );
    
    // 【功能】：保存容器到输出文件流（递归保存所有子原子）
    virtual void save ( std::fstream &, std::fstream &, int32_t );

public:
    // 【公有成员】：容器特定属性
    
    // 填充大小：用于样本描述等特定格式的字节填充
    // - 对于STSD（样本描述）原子：基础填充8字节
    // - 对于音频样本描述：根据版本设置不同填充（28/44/64字节）
    uint32_t m_iPadding;
    
    // 子原子列表：存储容器中包含的所有原子
    // - 支持基础原子（Box）和嵌套容器（Container）
    // - 形成MP4文件的层次结构树
    std::vector<Box *> m_listContents;
};
