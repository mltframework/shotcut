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

// Tool for loading mpeg4 files and manipulating atoms.
#include <stdint.h>
#include <vector>

#include <fstream>

class Box 
{
  public:
    Box ( );
    virtual ~Box ( );
    virtual int32_t type ( );

    static Box *load  ( std::fstream &, uint32_t, uint32_t );
    static void clear ( std::vector<Box *> & );

    int content_start ( );
    virtual void save ( std::fstream &, std::fstream &, int32_t );
    void set  ( uint8_t *, uint32_t );
    int  size ( );
    const char *name( );
    virtual void print_structure ( const char * );
    void tag_copy   ( std::fstream &, std::fstream &, int32_t );
    void index_copy ( std::fstream &, std::fstream &, Box *, bool, int32_t );
    void stco_copy  ( std::fstream &, std::fstream &, Box *, int32_t );
    void co64_copy  ( std::fstream &, std::fstream &, Box *, int32_t );

  public:
    static   int8_t readInt8   ( std::fstream &fs );
    static  int16_t readInt16  ( std::fstream &fs );
    static  int32_t readInt32  ( std::fstream &fs );
    static  uint8_t readUint8  ( std::fstream &fs );
    static uint32_t readUint32 ( std::fstream &fs );
    static uint64_t readUint64 ( std::fstream &fs );
    static double   readDouble ( std::fstream &fs );

    static void     writeInt16 ( std::fstream &fs, int16_t  );
    static void     writeInt32 ( std::fstream &fs, int32_t  );
    static void     writeUint8 ( std::fstream &fs, uint8_t  );
    static void     writeUint32( std::fstream &fs, uint32_t );
    static void     writeUint64( std::fstream &fs, uint64_t );

    int32_t  m_iType;

  private:
    uint32_t uint32FromCont ( int32_t &iIDX );
    uint64_t uint64FromCont ( int32_t &iIDX );
    void index_copy_from_contents ( std::fstream &fsOut, Box *pBox, bool bBigMode, int32_t iDelta );

  public: 
    char      m_name[4];
    uint32_t  m_iPosition;
    uint32_t  m_iHeaderSize;
    uint32_t  m_iContentSize;
    uint8_t  *m_pContents;
};

