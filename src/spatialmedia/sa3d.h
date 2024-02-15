#pragma once
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

#include <stdint.h>

#include <fstream>
#include <vector>
#include <map>

#include "box.h"

class SA3DBox : public Box
{
  public:
    enum ePosition { None };

    SA3DBox ();
    virtual ~SA3DBox ( );

    // Loads the SA3D box located at position pos in a mp4 file.
    static Box *load ( std::fstream &fs, uint32_t iPos, uint32_t iEnd );

    static Box *create ( int32_t iNumChannels );

    virtual void save ( std::fstream &fsIn, std::fstream &fsOut, int32_t );
    const char *ambisonic_type_name ( );
    const char *ambisonic_channel_ordering_name ( );
    const char *ambisonic_normalization_name ( );
    void print_box ( );

    std::string get_metadata_string ( );

  private:
    std::string mapToString ( );

  public:
    std::map<std::string, int32_t> m_AmbisonicTypes;
    std::map<std::string, int32_t> m_AmbisonicOrderings;
    std::map<std::string, int32_t> m_AmbisonicNormalizations;

//    int32_t  m_iPosition;
    uint8_t  m_iVersion;
    uint8_t  m_iAmbisonicType;
    uint32_t m_iAmbisonicOrder;
    uint8_t  m_iAmbisonicChannelOrdering;
    uint8_t  m_iAmbisonicNormalization;
    uint32_t m_iNumChannels;
    std::vector<uint32_t> m_ChannelMap;
};

