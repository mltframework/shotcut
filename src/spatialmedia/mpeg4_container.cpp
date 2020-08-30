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
#include <string.h>
#include <stdlib.h>

#include "mpeg4_container.h"

Mpeg4Container::Mpeg4Container ( )
  : Container ( )
{
  m_pMoovBox      = NULL;
  m_pFreeBox      = NULL;
  m_pFirstMDatBox = NULL;
  m_pFTYPBox      = NULL;
  m_iFirstMDatPos = 0;
}

Mpeg4Container::~Mpeg4Container ( )
{

}

Mpeg4Container *Mpeg4Container::load ( std::fstream &fsIn ) //, uint32_t /* iPos */, uint32_t /* iEnd */ )
{
  // Load the mpeg4 file structure of a file.
//  fsIn.seekg ( 0, 2 );
  int32_t iSize = fsIn.tellg ( );
  std::vector<Box *> list = load_multiple ( fsIn, 0, iSize );

  if ( list.empty ( ) )  {
    std::cerr << "Error, failed to load .mp4 file." << std::endl;
    return NULL; 
  }
  Mpeg4Container *pNewBox = new Mpeg4Container ( );
  pNewBox->m_listContents = list;

  std::vector<Box *>::iterator it = list.begin ( );
  while ( it != list.end ( ) )  {
    Box *pBox = *it++;
    if ( memcmp ( pBox->m_name, "moov", 4 ) == 0 )
      pNewBox->m_pMoovBox = pBox;
    if ( memcmp ( pBox->m_name, "free", 4 ) == 0 )
      pNewBox->m_pFreeBox = pBox;
    if ( ( memcmp ( pBox->m_name, "mdat", 4 ) == 0 ) &&
         ( ! pNewBox->m_pFirstMDatBox ) )
      pNewBox->m_pFirstMDatBox = (Mpeg4Container *)pBox;
    if ( memcmp ( pBox->m_name, "ftyp", 4 ) == 0 )
      pNewBox->m_pFTYPBox = pBox;
  }
  if ( ! pNewBox->m_pMoovBox )  {
    std::cerr << "Error, file does not contain moov box." << std::endl;
    delete pNewBox;
    return NULL;
  }
  if ( ! pNewBox->m_pFirstMDatBox )  {
    std::cerr << "Error, file does not contain mdat box." << std::endl;
    delete pNewBox;
    return NULL;
  }
  pNewBox->m_iFirstMDatPos  = pNewBox->m_pFirstMDatBox->m_iPosition; //m_iFirstMDatPos;
  pNewBox->m_iFirstMDatPos += pNewBox->m_pFirstMDatBox->m_iHeaderSize;
  pNewBox->m_iContentSize   = 0;
  it = pNewBox->m_listContents.begin ( );
  while ( it != pNewBox->m_listContents.end ( ) )  {
    Box *pBox = *it++;
    pNewBox->m_iContentSize += pBox->size ( );
  }
  return pNewBox;
}

void Mpeg4Container::merge ( Box *pElement )
{
  // Mpeg4 containers do not support merging."""
  std::cerr << "Cannot merge mpeg4 files" << std::endl;
  exit ( 0 );
}

void Mpeg4Container::print_structure ( const char *pIndent )
{
  // Print mpeg4 file structure recursively."""
  std::cout << "mpeg4 [" << m_iContentSize << "]" << std::endl;
  uint32_t iCount = m_listContents.size ( );
  std::string strIndent = pIndent;
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end () )  {
    Box *pBox = *it++;
    strIndent = " ├──";
    if ( --iCount <= 0 )
      strIndent = " └──";
    pBox->print_structure ( strIndent.c_str ( ) );
  }
}

void Mpeg4Container::save ( std::fstream &fsIn, std::fstream &fsOut, int32_t )
{
  // Save mpeg4 filecontent to file.
  resize ( );
  uint32_t iNewPos = 0;
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  while ( it != m_listContents.end () )  {
    Box *pBox = *it++;
    if ( memcmp( pBox->m_name, constants::TAG_MDAT, 4 ) == 0 )  {
      iNewPos += pBox->m_iHeaderSize;
      break;
    }
    iNewPos += pBox->size ( );
  }
  uint32_t iDelta = iNewPos - m_iFirstMDatPos;
  it = m_listContents.begin ( );
  while ( it != m_listContents.end () )  {
    Box *pBox = *it++; 
    pBox->save ( fsIn, fsOut, iDelta );
  }
}

