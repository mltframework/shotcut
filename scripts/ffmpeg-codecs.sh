#!/bin/sh

ffmpeg -codecs -hide_banner | awk '(substr($1,2,1) == "E" && substr($1,3,1) == "V" && substr($1,4,1) == "I") {print "m_intraOnlyCodecs << \"" $2 "\";"}'
echo
ffmpeg -codecs -hide_banner | awk '(substr($1,2,1) == "E" && substr($1,3,1) == "V" && substr($1,5,2) == ".S") {print "m_losslessVideoCodecs << \"" $2 "\";"}'
echo
ffmpeg -codecs -hide_banner | awk '(substr($1,2,1) == "E" && substr($1,3,1) == "A" && substr($1,5,2) == ".S") {print "m_losslessAudioCodecs << \"" $2 "\";"}'
