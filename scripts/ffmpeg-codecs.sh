#!/bin/sh

ffmpeg -codecs -hide_banner | awk '(substr($1,2,1) == "E" && substr($1,3,1) == "V" && substr($1,4,1) == "I") {
  i = index($0, "(encoders:")
  if (i > 0) {
    split(substr($0, i), a)
    for (i = 2; i < length(a); i++)
      print "m_intraOnlyCodecs << \"" a[i] "\";"
  } else {
    print "m_intraOnlyCodecs << \"" $2 "\";"
  }
}'
echo
ffmpeg -codecs -hide_banner | awk '(substr($1,2,1) == "E" && substr($1,3,1) == "V" && substr($1,5,2) == ".S") {
  i = index($0, "(encoders:")
  if (i > 0) {
    split(substr($0, i), a)
    for (i = 2; i < length(a); i++)
      print "m_losslessVideoCodecs << \"" a[i] "\";"
  } else {
    print "m_losslessVideoCodecs << \"" $2 "\";"
  }
}'
echo
ffmpeg -codecs -hide_banner | awk '(substr($1,2,1) == "E" && substr($1,3,1) == "A" && substr($1,5,2) == ".S") {
  i = index($0, "(encoders:")
  if (i > 0) {
    split(substr($0, i), a)
    for (i = 2; i < length(a); i++)
      print "m_losslessAudioCodecs << \"" a[i] "\";"
  } else {
    print "m_losslessAudioCodecs << \"" $2 "\";"
  }
}'
