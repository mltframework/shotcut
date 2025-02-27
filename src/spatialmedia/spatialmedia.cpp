/*****************************************************************************
 *
 * Copyright 2016 Varol Okan. All rights reserved.
 * Copyright (c) 2020-2024 Meltytech, LLC
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
#include "sa3d.h"

#include <stdint.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "Logger.h"

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

static int get_descriptor_length(std::fstream &inFile)
{
    auto result = 0;
    uint8_t size_byte;

    for (int i = 0; i < 4; i++) {
        inFile.read((char*) &size_byte, 1);
        result = (result << 7) | (size_byte & 0x7f);
        if (size_byte != 0x80) break;
    }
    return result;
}

static int get_aac_num_channels(Box *mp4aBox, std::fstream &inFile)
{
    auto result = -1;
    auto size = sizeof(mp4aBox->m_name);
    auto pos = inFile.tellg();

    for (auto box : static_cast<Container*>(mp4aBox)->m_listContents) {
        if (!memcmp(constants::TAG_WAVE, box->m_name, size)) {
            // Handle .mov with AAC audio: stsd -> mp4a -> wave -> esds
            return get_aac_num_channels(box, inFile);
        } else if (!memcmp(constants::TAG_ESDS, box->m_name, size)) {
            // Read the AAC AudioSpecificConfig
            char data[2];
            inFile.seekg(box->content_start() + 4);
            // Verify the read descriptor is an elementary stream descriptor
            inFile.read(data, 1);
            if (data[0] != 3) break;
            // Verify the read descriptor is a decoder config. descriptor
            auto length = get_descriptor_length(inFile);
            inFile.seekg(3, std::ios_base::cur);
            inFile.read(data, 1);
            if (data[0] != 4) break;
            //  Verify the read descriptor is a decoder specific info descriptor
            length = get_descriptor_length(inFile);
            inFile.seekg(13, std::ios_base::cur); // offset to the decoder specific config descriptor
            inFile.read(data, 1);
            if (data[0] != 5) break;
            auto audio_specific_descriptor_size = get_descriptor_length(inFile);
            if (audio_specific_descriptor_size < 2) break;
            inFile.read(data, 2);
            auto object_type = (data[0] >> 3) & 0x1f;
            if (object_type != 2) break;
            auto sampling_frequency_index = ((data[0] & 0x07) << 1 | (data[1] >> 7) & 0x01);
            // TODO: If the sample rate is 96kHz an additional 24 bit offset
            // value here specifies the actual sample rate.
            if (sampling_frequency_index == 0) break;
            result = (data[1] >> 3) & 0x0f;
        }
    }
    inFile.seekg(pos);
    return result;
}

static bool sound_samples_contains(const char *name)
{
    auto nameSize = sizeof(Box::m_name);
    auto size = sizeof(constants::SOUND_SAMPLE_DESCRIPTIONS) / nameSize;
    for (int i = 0; i < size; i++) {
        if (!memcmp(name, constants::SOUND_SAMPLE_DESCRIPTIONS[i], nameSize))
            return true;
    }
    return false;
}

static int get_sample_description_num_channels(Box *ssdBox, std::fstream &inFile)
{
    auto result = -1;
    auto size = sizeof(ssdBox->m_name);
    auto pos = inFile.tellg();
    char data[4];

    // Read the AAC AudioSpecificConfig
    inFile.seekg(ssdBox->content_start() + 8);
    inFile.read(data, 2);
    auto version = (data[0] << 8) | data[1];
    inFile.seekg(2 + 4, std::ios_base::cur); // revision_level and vendor
    switch (version) {
    case 0:
    case 1:
        inFile.read(data, 2);
        result = (data[0] << 8) | data[1];
        break;
    case 2:
        inFile.seekg(24, std::ios_base::cur);
        inFile.read(data, 4);
        result = 0;
        for (int i = 0; i < 4; i++)
            result = (result << 8) | data[i];
        break;
    }
    inFile.seekg(pos);
    return result;
}

static void mpeg4_add_spatial_audio(Box *mdiaBox, std::fstream &inFile)
{
    auto size = sizeof(mdiaBox->m_name);
    for (auto box : static_cast<Container*>(mdiaBox)->m_listContents) {
        if (!memcmp(constants::TAG_MINF, box->m_name, size)) {
            for (auto box : static_cast<Container*>(box)->m_listContents) {
                if (!memcmp(constants::TAG_STBL, box->m_name, size)) {
                    for (auto box : static_cast<Container*>(box)->m_listContents) {
                        if (!memcmp(constants::TAG_STSD, box->m_name, size)) {
                            for (auto box : static_cast<Container*>(box)->m_listContents) {
                                auto channels = 0;
                                if (!memcmp(constants::TAG_MP4A, box->m_name, size)) {
                                    channels = get_aac_num_channels(box, inFile);
                                } else if (sound_samples_contains(box->m_name)) {
                                    channels = get_sample_description_num_channels(box, inFile);
                                }
                                if (4 == channels) {
                                    static_cast<Container*>(box)->add(SA3DBox::create(channels));
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
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
          if  ( memcmp ( pMDIA->m_name, constants::TAG_MINF, 4 ) == 0 ) {
              mpeg4_add_spatial_audio(pSub, inFile);
              continue;
          }
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

