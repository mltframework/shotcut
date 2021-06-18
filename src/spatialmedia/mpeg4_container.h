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

// MPEG4 processing classes.
// 
// Functions for loading MP4/MOV files and manipulating boxes.

#include "constants.h"
#include "container.h"
#include "box.h"

class Mpeg4Container : public Container
{
  public:
    Mpeg4Container ( );
    virtual ~Mpeg4Container ( );

    static Mpeg4Container *load ( std::fstream & ); //, uint32_t iPos, uint32_t iEnd );

    void merge ( Box * );
    virtual void print_structure ( const char *p="" );
    virtual void save ( std::fstream &, std::fstream &, int32_t );

public:
  Box *m_pMoovBox;
  Box *m_pFreeBox;
  Box *m_pFTYPBox;
  Mpeg4Container *m_pFirstMDatBox;
  uint32_t m_iFirstMDatPos;
};

