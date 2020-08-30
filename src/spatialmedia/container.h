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

// MPEG processing classes.
//
// Functions for loading MPEG files and manipulating boxes.
#include <vector>

#include "box.h"

class Container : public Box
{
  public:
    Container ( uint32_t iPadding=0 );
    virtual ~Container ( );

    static Box *load ( std::fstream &, uint32_t iPos, uint32_t iEnd );
    static std::vector<Box *>load_multiple ( std::fstream &, uint32_t iPos, uint32_t iEnd );

    void resize ( );
    virtual void print_structure ( const char * );
    void remove ( const char * );
    bool add    ( Box * );
    bool merge  ( Box * );
    virtual void save ( std::fstream &, std::fstream &, int32_t );

public:
    uint32_t m_iPadding;
    std::vector<Box *> m_listContents;
};

