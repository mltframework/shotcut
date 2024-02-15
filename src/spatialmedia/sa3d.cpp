/*****************************************************************************
 * 
 * Copyright 2016 Varol Okan. All rights reserved.
 * Copyright (c) 2024 Meltytech, LLC
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

/* MPEG SA3D Box processing class
 * This class enables the injection of SA2D MPEG-4. The SA3D box 
 * specification as outlned in 
 * https://github.com/google/spatial-media/docs/spatial-audio-rfc.md
 */

#include <string.h>
#include <iostream>
#include <sstream>
#include <iterator>
#include <cmath>

#include "constants.h"
#include "sa3d.h"

SA3DBox::SA3DBox ()
  : Box ( )
{
  memcpy ( m_name, constants::TAG_SA3D, 4 );
  m_iHeaderSize    = 8;
  m_iPosition      = 0;
  m_iContentSize   = 0;
  m_iVersion       = 0;
  m_iAmbisonicType = 0;
  m_iAmbisonicOrder= 0;
  m_iAmbisonicChannelOrdering = 0;
  m_iAmbisonicNormalization   = 0;
  m_iNumChannels   = 0;

  m_AmbisonicTypes["periphonic"]    = 0;
  m_AmbisonicOrderings["ACN"]       = 0;
  m_AmbisonicNormalizations["SN3D"] = 0;
}

SA3DBox::~SA3DBox ( )
{
}

// Loads the SA3D box located at position pos in a mp4 file.
Box *SA3DBox::load ( std::fstream &fs, uint32_t iPos, uint32_t iEnd )
{
  SA3DBox *pNewBox = NULL;
  if ( iPos < 0 )
       iPos = fs.tellg ( );

  char name[4];

  fs.seekg( iPos );
  uint32_t iSize = readUint32 ( fs );
  fs.read ( name, 4 );
  // Test if iSize == 1
  // Added for 360Tube to have load and save in-sync.
  if ( iSize == 1 )  { 
    iSize = (int32_t)readUint64 ( fs );
  }

  if ( 0 != memcmp ( name, constants::TAG_SA3D, sizeof ( *constants::TAG_SA3D ) ) )  {
    std::cerr << "Error: box is not an SA3D box." << std::endl;
    return NULL;
  }

  if ( iPos + iSize > iEnd )  {
    std::cerr << "Error: SA3D box size exceeds bounds." << std::endl;
    return NULL;
  }

  pNewBox = new SA3DBox ( );
  pNewBox->m_iPosition = iPos;
  pNewBox->m_iContentSize    = iSize - pNewBox->m_iHeaderSize;
  pNewBox->m_iVersion        = readUint8  ( fs );
  pNewBox->m_iAmbisonicType  = readUint8  ( fs );
  pNewBox->m_iAmbisonicOrder = readUint32 ( fs );
  pNewBox->m_iAmbisonicChannelOrdering = readUint8 ( fs );
  pNewBox->m_iAmbisonicNormalization   = readUint8 ( fs );
  pNewBox->m_iNumChannels    = readUint32 ( fs );

  for ( auto i = 0U; i< pNewBox->m_iNumChannels; i++ )  {
    uint32_t iVal = readUint32 ( fs );
    pNewBox->m_ChannelMap.push_back ( iVal );
  }
  return pNewBox;
}

Box *SA3DBox::create (int32_t iNumChannels)
{
  // audio_metadata: dictionary ('ambisonic_type': string, 'ambisonic_order': int),

  SA3DBox *pNewBox = new SA3DBox ( );
  pNewBox->m_iHeaderSize   = 8;
  memcpy ( pNewBox->m_name, constants::TAG_SA3D, 4 );
  pNewBox->m_iAmbisonicOrder = ::sqrt(iNumChannels) - 1;

  pNewBox->m_iVersion       = 0; // # uint8
  pNewBox->m_iContentSize += 1; // # uint8
//  pNewBox->m_iAmbisonicType= pNewBox->m_AmbisonicTypes[amData["ambisonic_type"]];
  pNewBox->m_iContentSize += 1; // # uint8
//  pNewBox->m_iAmbisonicOrder = amData["ambisonic_order"];
  pNewBox->m_iContentSize += 4; // # uint32
//  pNewBox->m_iAmbisonicChannelOrdering = pNewBox->m_AmbisonicOrderings[ amData["ambisonic_channel_ordering"]]
  pNewBox->m_iContentSize += 1; // # uint8
//  pNewBox->m_iAmbisonicNormalization = pNewBox->m_AmbisonicNormalizations[ amData["ambisonic_normalization"]]
  pNewBox->m_iContentSize += 1; // # uint8
  pNewBox->m_iNumChannels = iNumChannels;
  pNewBox->m_iContentSize += 4; // # uint32

  // std::vector<int> map; // = amData["channel_map"];
  // std::vector<int>::iterator it = map.begin ( );
  // while ( it != map.end ( ) )  {
  //   pNewBox->m_ChannelMap.push_back ( *it++ );
  //   pNewBox->m_iContentSize += 4;
  // }
  for (uint32_t i = 0; i < iNumChannels; i++) {
    pNewBox->m_ChannelMap.push_back(i);
    pNewBox->m_iContentSize += 4; // # uint32
  }
  return pNewBox;
}

void SA3DBox::save (std::fstream &fsIn, std::fstream &fsOut , int32_t)
{
  (void) fsIn; // unused
  //char tmp, name[4];
 
  if ( m_iHeaderSize == 16 )  {
    writeUint32 ( fsOut, 1 );
    fsOut.write ( m_name,  4 );
    writeUint64 ( fsOut, size() );
    //fsOut.write ( name,  4 ); I think this is a bug in the original code here.
  }
  else if ( m_iHeaderSize == 8 )  {
    writeUint32 ( fsOut, size() );
    fsOut.write ( m_name, 4 );
  }

  writeUint8  ( fsOut, m_iVersion );
  writeUint8  ( fsOut, m_iAmbisonicType );
  writeUint32 ( fsOut, m_iAmbisonicOrder );
  writeUint8  ( fsOut, m_iAmbisonicChannelOrdering );
  writeUint8  ( fsOut, m_iAmbisonicNormalization );
  writeUint32 ( fsOut, m_iNumChannels );

  std::vector<uint32_t>::iterator it = m_ChannelMap.begin ( );
  while ( it != m_ChannelMap.end ( ) )  {
    writeUint32 ( fsOut, *it++ );
  }
}

const char *SA3DBox::ambisonic_type_name ( )
{
  //return  (key for key,value in SA3DBox.ambisonic_types.items()
  //  if value==self.ambisonic_type).next()
  return NULL;
}

const char *SA3DBox::ambisonic_channel_ordering_name ( )
{
  //return (key for key,value in SA3DBox.ambisonic_orderings.items()
  //  if value==self.ambisonic_channel_ordering).next()
  return NULL;
}

const char * SA3DBox::ambisonic_normalization_name ( )
{
  //return (key for key,value in SA3DBox.ambisonic_normalizations.items()
  //  if value==self.ambisonic_normalization).next()
  return NULL;
}

std::string SA3DBox::mapToString ( )
{
  std::stringstream ss;
  std::copy ( m_ChannelMap.begin ( ), m_ChannelMap.end ( ), std::ostream_iterator<uint32_t>(ss, ", " ) );
  return ss.str ( );
}

void SA3DBox::print_box ( )
{
  // Prints the contents of this spatial audio (SA3D) box to the console.
  const char *ambisonic_type   = ambisonic_type_name ( );
  const char *channel_ordering = ambisonic_channel_ordering_name ( );
  const char *ambisonic_normalization = ambisonic_normalization_name ( );
  std::string str = mapToString ( );

  std::cout << "\t\tAmbisonic Type: " << ambisonic_type << std::endl;
  std::cout << "\t\tAmbisonic Order: " << m_iAmbisonicOrder << std::endl;
  std::cout << "\t\tAmbisonic Channel Ordering: " << channel_ordering << std::endl;
  std::cout << "\t\tAmbisonic Normalization: " << ambisonic_normalization << std::endl;
  std::cout << "\t\tNumber of Channels: " << m_iNumChannels << std::endl;
  std::cout << "\t\tChannel Map: %s" << str << std::endl;
}

std::string SA3DBox::get_metadata_string ( )
{
  // Outputs a concise single line audio metadata string.
  std::ostringstream str;
  str << ambisonic_normalization_name ( ) << ", ";
  str << ambisonic_channel_ordering_name ( ) << ", ";
  str << ambisonic_type_name ( ) << ", Order ";
  str << m_iAmbisonicOrder << ", ";
  str << m_iNumChannels << ", Channel(s), Channel Map: ";
  str << mapToString ( );

  return str.str ( );
}


