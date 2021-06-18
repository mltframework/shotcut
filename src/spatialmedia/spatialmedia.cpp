/*****************************************************************************
 *
 * Copyright 2016 Varol Okan. All rights reserved.
 * Copyright (c) 2020 Meltytech, LLC
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

#include "spatialmedia.h"
#include "mpeg4_container.h"

#include <stdint.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <Logger.h>

static const uint8_t SPHERICAL_UUID_ID[] = {0xff, 0xcc, 0x82, 0x63, 0xf8, 0x55, 0x4a, 0x93, 0x88, 0x14, 0x58, 0x7a, 0x02, 0x52, 0x1f, 0xdd };
//    "\xff\xcc\x82\x63\xf8\x55\x4a\x93\x88\x14\x58\x7a\x02\x52\x1f\xdd")

//static std::string RDF_PREFIX = " xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" ";

static std::string SPHERICAL_XML_HEADER = "<?xml version=\"1.0\"?>"\
    "<rdf:SphericalVideo\n"\
    "xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"\
    "xmlns:GSpherical=\"http://ns.google.com/videos/1.0/spherical/\">";

static std::string SPHERICAL_XML_CONTENTS = "<GSpherical:Spherical>true</GSpherical:Spherical>"\
    "<GSpherical:Stitched>true</GSpherical:Stitched>"\
    "<GSpherical:StitchingSoftware>Spherical Metadata Tool</GSpherical:StitchingSoftware>"\
    "<GSpherical:ProjectionType>equirectangular</GSpherical:ProjectionType>";

//static std::string SPHERICAL_XML_CONTENTS_TOP_BOTTOM = "<GSpherical:StereoMode>top-bottom</GSpherical:StereoMode>";
//static std::string SPHERICAL_XML_CONTENTS_LEFT_RIGHT = "<GSpherical:StereoMode>left-right</GSpherical:StereoMode>";

// Parameter order matches that of the crop option.
static std::string SPHERICAL_XML_CONTENTS_CROP_FORMAT = \
    "<GSpherical:CroppedAreaImageWidthPixels>%d</GSpherical:CroppedAreaImageWidthPixels>"\
    "<GSpherical:CroppedAreaImageHeightPixels>%d</GSpherical:CroppedAreaImageHeightPixels>"\
    "<GSpherical:FullPanoWidthPixels>%d</GSpherical:FullPanoWidthPixels>"\
    "<GSpherical:FullPanoHeightPixels>%d</GSpherical:FullPanoHeightPixels>"\
    "<GSpherical:CroppedAreaLeftPixels>%d</GSpherical:CroppedAreaLeftPixels>"\
    "<GSpherical:CroppedAreaTopPixels>%d</GSpherical:CroppedAreaTopPixels>";

static std::string SPHERICAL_XML_FOOTER = "</rdf:SphericalVideo>";
//static std::string SPHERICAL_PREFIX = "{http://ns.google.com/videos/1.0/spherical/}";

static Box *spherical_uuid ( std::string &strMetadata )
{
  // Constructs a uuid containing spherical metadata.
  Box *p = new Box;
  // a box containing spherical metadata.
//  if ( strUUID.length ( ) != 16 )
//    std::cerr << "ERROR: Data mismatch" << std::endl;
  int iSize = strMetadata.size ( );
  const uint8_t *pMetadata = reinterpret_cast<const uint8_t*>(strMetadata.c_str());

  memcpy ( p->m_name, constants::TAG_UUID, 4 );
  p->m_iHeaderSize  = 8;
  p->m_iContentSize = 0;
  p->m_pContents    = new uint8_t[iSize + 16 + 1];
  memcpy ( p->m_pContents, SPHERICAL_UUID_ID, 16 );
  memcpy ((p->m_pContents+16),  pMetadata, iSize );
  p->m_iContentSize=iSize+16;

  return p;
}

static bool mpeg4_add_spherical ( Mpeg4Container *pMPEG4, std::fstream &inFile, std::string &strMetadata )
{
  // Adds a spherical uuid box to an mpeg4 file for all video tracks.
  //
  // pMPEG4 : Mpeg4 file structure to add metadata.
  // inFile : file handle, Source for uncached file contents.
  // strMetadata: string, xml metadata to inject into spherical tag.
  if ( ! pMPEG4 )
    return false;

  bool bAdded = false;
  Container *pMoov = (Container *)pMPEG4->m_pMoovBox;
  if ( ! pMoov )
    return false;

  std::vector<Box *>::iterator it = pMoov->m_listContents.begin ( );
  while ( it != pMoov->m_listContents.end ( ) )  {
    Container *pBox = (Container *)*it++;
    if ( memcmp ( pBox->m_name, constants::TAG_TRAK, 4 ) == 0 )  {
      bAdded = false;
      pBox->remove ( constants::TAG_UUID );

      std::vector<Box *>::iterator it2 = pBox->m_listContents.begin ( );
      while ( it2 != pBox->m_listContents.end ( ) )  {
        Container *pSub = (Container *)*it2++;
        if ( memcmp ( pSub->m_name, constants::TAG_MDIA, 4 ) != 0 )
          continue;

        std::vector<Box *>::iterator it3 = pSub->m_listContents.begin ( );
        while ( it3 != pSub->m_listContents.end ( ) )  {
          Box *pMDIA = *it3++;
          if  ( memcmp ( pMDIA->m_name, constants::TAG_HDLR, 4 ) != 0 )
            continue;

          char name[4];
          int iPos = pMDIA->content_start ( ) + 8;
          inFile.seekg( iPos );
          inFile.read ( name, 4 );
          if ( memcmp ( name, constants::TRAK_TYPE_VIDE, 4 ) == 0 )  {
            bAdded = true;
            break;
          }
        }
        if ( bAdded )  {
          if ( ! pBox->add ( spherical_uuid ( strMetadata ) ) )
            return true;
          break;
        }
      }
    }
  }
  pMPEG4->resize ( );
  return true;
}

bool SpatialMedia::injectSpherical(const std::string& strInFile, const std::string& strOutFile)
{
    std::fstream inFile(strInFile.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) {
        LOG_ERROR() << "Error \"" << strInFile.c_str() << "\" does not exist or do not have permission.";
        return false;
    }
    Mpeg4Container* pMPEG4 = Mpeg4Container::load(inFile);
    if (!pMPEG4)  {
        LOG_ERROR() << "Error, file could not be opened.";
        return false;
    }
    std::string stereo_xml;
//    if ( stereo == SpatialMedia::Parser::SM_TOP_BOTTOM )
//      stereo_xml += SPHERICAL_XML_CONTENTS_TOP_BOTTOM;
//    if ( stereo == SpatialMedia::Parser::SM_LEFT_RIGHT )
//      stereo_xml += SPHERICAL_XML_CONTENTS_LEFT_RIGHT;
    std::string xml = SPHERICAL_XML_HEADER + SPHERICAL_XML_CONTENTS + stereo_xml + SPHERICAL_XML_FOOTER;;
    bool bRet = mpeg4_add_spherical(pMPEG4, inFile, xml);
    if (!bRet) {
        LOG_ERROR() << "Error failed to insert spherical data";
    }
    LOG_INFO() << "Saved spatial media metadata";

    std::fstream outFile(strOutFile.c_str(), std::ios::out | std::ios::binary);
    if (!outFile.is_open())  {
        LOG_ERROR() << "Error file: \"" << strOutFile.c_str() << "\" could not create or do not have permission.";
        return false;
    }
    pMPEG4->save(inFile, outFile, 0);
    return true;
}

