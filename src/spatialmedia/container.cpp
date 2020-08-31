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

#include <iostream>
#include <assert.h>
#include <string.h>

#include "constants.h"
#include "container.h"

#include "sa3d.h"


Container::Container ( uint32_t iPadding )
   : Box ( )
{
  m_iType = constants::Container;
  m_iPadding = iPadding;
}

Container::~Container ( )
{
}

Box *Container::load ( std::fstream &fs, uint32_t iPos, uint32_t iEnd )
{
//  if ( iPos == 0 )
//       iPos = fs.tellg ( );

  fs.seekg ( iPos );
  uint32_t iHeaderSize = 8;
  uint32_t t, iSize = readUint32 ( fs );
  char name[4];
  fs.read ( name, 4 );

  int32_t iArrSize = (int32_t)(sizeof(constants::CONTAINERS_LIST)/sizeof(constants::CONTAINERS_LIST[0]));
  bool bIsBox = true;
  for ( t=0; t<iArrSize; t++ )  {
    if ( memcmp ( name, constants::CONTAINERS_LIST[t], 4 ) == 0 )  {
      bIsBox = false;
      break;
    }
  }

  // Handle the mp4a decompressor setting (wave -> mp4a).
  if ( memcmp ( name, constants::TAG_MP4A, 4 ) == 0 && iSize == 12 ) 
    bIsBox = true;

  if ( bIsBox )  {
    if ( memcmp ( name, constants::TAG_SA3D, 4 ) == 0 )
      return SA3DBox::load ( fs, iPos, iEnd );
    return Box::load ( fs, iPos, iEnd );
  }

  if( iSize == 1 )  {
    iSize = (uint32_t)readUint64 ( fs );
    iHeaderSize = 16;
  }

  if ( iSize < 8 )  {
    std::cerr << "Error, invalid size " << iSize << " in " << name << " at " << iPos << std::endl;
    return NULL;
  }

  if ( iPos + iSize > iEnd )  {
    std::cerr << "Error: Container box size exceeds bounds." << std::endl;
    return NULL;
  }

  uint32_t iPadding = 0;
  if ( memcmp ( name, constants::TAG_STSD, 4 ) == 0 )
    iPadding = 8;

  uint32_t iCurrentPos = 0;
  int16_t  iSampleDescVersion = 0;
  iArrSize = (int32_t)(sizeof(constants::SOUND_SAMPLE_DESCRIPTIONS)/sizeof(constants::SOUND_SAMPLE_DESCRIPTIONS[0]));
  for ( t=0; t<iArrSize; t++ )  {
    if ( memcmp ( name, constants::SOUND_SAMPLE_DESCRIPTIONS[t], 4 ) == 0 )  {
      iCurrentPos = fs.tellg ( );
      fs.seekg ( iCurrentPos + 8 );
      fs.read  ( (char *)&iSampleDescVersion, 2 );
      iSampleDescVersion = be16toh ( iSampleDescVersion );
      fs.seekg ( iCurrentPos );

      switch ( iSampleDescVersion )  {
      case 0:
        iPadding = 28;
      break;
      case 1:
        iPadding = 28 + 16;
      break;
      case 2:
        iPadding = 64;
      break;
      default:
        std::cerr << "Unsupported sample description version:" << iSampleDescVersion << std::endl;
      break;
      }
    }
  }
  Container *pNewBox = new Container ( );
  memcpy ( pNewBox->m_name, name, 4 );
  pNewBox->m_iPosition    = iPos;
  pNewBox->m_iHeaderSize  = iHeaderSize;
  pNewBox->m_iContentSize = iSize - iHeaderSize;
  pNewBox->m_iPadding     = iPadding;
  pNewBox->m_listContents = load_multiple ( fs, iPos + iHeaderSize + iPadding, iPos + iSize );

  if ( pNewBox->m_listContents.empty ( ) )  {
    delete pNewBox;
    return NULL;
  }

  return pNewBox;
}

std::vector<Box *> Container::load_multiple ( std::fstream &fs, uint32_t iPos, uint32_t iEnd )
{
  std::vector<Box *> list, empty;
  while ( iPos < iEnd )  {
    Box *pBox = load ( fs, iPos, iEnd );
    if ( ! pBox )  {
      std::cerr << "Error, failed to load box." << std::endl;
      clear ( list );
      return empty;
    }
    list.push_back ( pBox );
    iPos = pBox->m_iPosition + pBox->size ( );
  }
  return list;
}

void Container::resize ( )
{
  // Recomputes the box size and recurses on contents."""
  m_iContentSize = m_iPadding;
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pBox = *it++;
    if ( pBox->type ( ) == constants::Container )  {
      Container *p = (Container *)pBox;
      p->resize ( );
    }
    m_iContentSize += pBox->size ( );
  }
}

void Container::print_structure ( const char *pIndent )
{
  // Prints the box structure and recurses on contents."""
  uint32_t iSize1 = m_iHeaderSize;
  uint32_t iSize2 = m_iContentSize;
  std::cout <<  "{" << pIndent << "} {" << name ( ) << "} [{" << iSize1 << "}, {" << iSize2 << "}]" << std::endl;

  int32_t iCount = m_listContents.size ( );
  std::string strIndent = pIndent;
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pBox = *it++;
    if ( ! pBox )
      continue;
    strIndent.replace ( strIndent.begin ( ), strIndent.end ( ), "├", "│" );
    strIndent.replace ( strIndent.begin ( ), strIndent.end ( ), "└", " " );
    strIndent.replace ( strIndent.begin ( ), strIndent.end ( ), "─", " " );

    if ( --iCount <= 0 )
      strIndent += " └──";
    else
      strIndent += " ├──";
    pBox->print_structure ( strIndent.c_str ( ) );
  }
}

void Container::remove ( const char *pName )
{
  std::vector<Box *> list;
  m_iContentSize = 0;
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pBox = *it++;
    if ( ! pBox )
      continue;
    if ( memcmp ( pName, pBox->m_name, 4 ) != 0 )  {
      list.push_back ( pBox );
  
      if ( pBox->type ( ) == constants::Container )  {
        Container *p = (Container *)pBox;
        p->remove ( pName );
      }
      m_iContentSize += pBox->size ( );
    }
    else
      delete pBox;
  }
  m_listContents = list;
}

bool Container::add ( Box *pElement )
{
  // Adds an element, merging with containers of the same type.
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pBox = *it++;
    if ( memcmp ( pElement->m_name, pBox->m_name, 4 ) == 0 )  {
      if ( pBox->type ( ) == constants::ContainerLeaf )  {
        Container *p = (Container *)pBox;
        return p->merge ( pElement );
      }
      std::cerr << "Error, cannot merge leafs." << std::endl;
      return false;
    }
  }
  m_listContents.push_back ( pElement );
  return true;
}

bool Container::merge ( Box *pElem )
{
  assert ( pElem->type ( ) == constants::Container ); // isinstance(element, container_box))
  Container *pElement = (Container *)pElem;
  // Merges structure with container.
  int iRet = memcmp ( m_name, pElement->m_name, 4 );
  assert ( iRet == 0 );
  std::vector<Box *>::iterator it = pElement->m_listContents.begin ( );
  while ( it != pElement->m_listContents.end ( ) )  {
    Box *pSubElement = *it++;
    if ( ! add ( pSubElement ) )
      return false;
  }
  return true;
}

void Container::save ( std::fstream &fsIn, std::fstream &fsOut, int32_t iDelta )
{
  // Saves box to out_fh reading uncached content from in_fh.
  // iDelta : file change size for updating stco and co64 files.
  if ( m_iHeaderSize == 16 )  {
    writeUint32 ( fsOut, 1 );
    fsOut.write ( m_name, 4 );
    writeUint64 ( fsOut, size ( ) );
  }
  else if ( m_iHeaderSize == 8 )  {
    writeUint32 ( fsOut, size ( ) );
    fsOut.write ( m_name, 4 );
  }
  if ( m_iPadding > 0 )  {
    fsIn.seekg ( content_start ( ) );
    Box::tag_copy ( fsIn, fsOut, m_iPadding );
  }

  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end ( ) )  {
    Box *pElement = *it++;
    if ( ! pElement )
      continue;
    pElement->save ( fsIn, fsOut, iDelta );
  }
}

