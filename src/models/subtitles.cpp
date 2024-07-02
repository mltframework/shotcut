/*
 * Copyright (c) 2024 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "subtitles.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static Subtitles::SubtitleVector readFromSrtStream(std::istream &stream)
{
    enum {
        STATE_SEEKING_NUM,
        STATE_READING_TIME,
        STATE_READING_TEXT,
    };

    std::string line;
    std::string text;
    int state = STATE_SEEKING_NUM;
    Subtitles::SubtitleItem item;
    Subtitles::SubtitleVector ret;

    while (std::getline(stream, line)) {

        switch (state) {

        case STATE_SEEKING_NUM: {
            state = STATE_READING_TIME;
            for (char &c : line) {
                if (!std::isdigit(c)) {
                    // Bad line. Keep seeking
                    state = STATE_SEEKING_NUM;
                    break;
                }
            }
            break;
        }

        case STATE_READING_TIME: {
            int sHours, sMinutes, sSeconds, sMiliseconds, eHours, eMinutes, eSeconds, eMiliseconds;
            const int ret = std::sscanf(line.c_str(), "%d:%d:%d,%d --> %d:%d:%d,%d", &sHours, &sMinutes,
                                        &sSeconds, &sMiliseconds, &eHours, &eMinutes, &eSeconds, &eMiliseconds);
            if (ret != 8) {
                state = STATE_SEEKING_NUM;
                break;
            }
            item.start = (((((sHours * 60) + sMinutes) * 60) + sSeconds) * 1000) + sMiliseconds;
            item.end = (((((eHours * 60) + eMinutes) * 60) + eSeconds) * 1000) + eMiliseconds;
            item.text.clear();
            state = STATE_READING_TEXT;
            break;
        }

        case STATE_READING_TEXT: {
            if (!line.empty()) {
                if (!item.text.empty()) {
                    item.text += "\n";
                }
                item.text += line;
            } else {
                ret.push_back(item);
                state = STATE_SEEKING_NUM;
            }
            break;
        }
        }
    }

    if (state == STATE_READING_TEXT) {
        ret.push_back(item);
    }

    return ret;
}

static std::string msToSrtTime(int64_t ms)
{
    int hours = std::floor(ms / 1000.0 / 60.0 / 60.0);
    int minutes = std::floor((ms - (hours * 60 * 60 * 1000)) / 1000.0 / 60.0);
    int seconds = std::floor((ms - ((hours * 60 + minutes) * 60 * 1000)) / 1000.0);
    int miliseconds = ms - (((hours * 60 + minutes) * 60 + seconds) * 1000);
    char buff[13];
    std::snprintf( buff, sizeof(buff), "%02d:%02d:%02d,%03d", hours, minutes, seconds, miliseconds );
    return std::string(buff);
}

static bool writeToSrtStream(std::ostream &stream, const Subtitles::SubtitleVector &items)
{
    if (items.size() == 0) {
        return true;
    }
    int i = 1;
    for (auto item : items) {
        stream << i << "\n";
        stream << msToSrtTime(item.start) << " --> " << msToSrtTime(item.end) << "\n";
        stream << item.text;
        if (!item.text.empty() && item.text.back() != '\n') {
            stream << "\n";
        }
        stream << "\n";
        i++;
    }
    return true;
}

Subtitles::SubtitleVector Subtitles::readFromSrtFile(const std::string &path)
{
    std::ifstream fileStream(path);
    return readFromSrtStream(fileStream);
}

bool Subtitles::writeToSrtFile(const std::string &path, const SubtitleVector &items)
{
    std::ofstream fileStream(path.c_str(), std::ios::out | std::ios::trunc);
    if (!fileStream.is_open()) {
        return false;
    }
    return writeToSrtStream(fileStream, items);
}

Subtitles::SubtitleVector Subtitles::readFromSrtString(const std::string &text)
{
    std::istringstream textStream(text);
    return readFromSrtStream(textStream);
}

bool Subtitles::writeToSrtString(std::string &text, const Subtitles::SubtitleVector &items)
{
    std::ostringstream textStream;
    bool result = writeToSrtStream(textStream, items);
    text = textStream.str();
    return result;
}

int Subtitles::indexForTime(const Subtitles::SubtitleVector &items, int64_t msTime, int searchStart)
{
    // Return -1 if there is no subtitle for the time.
    int index = -1;
    int count = (int)items.size();
    if (count == 0) {
        // Nothing to search
    } else if (count > 0 && items[0].start > msTime) {
        // No text if before the first item;
    } else if (count > 1 && items[count - 1].end < msTime) {
        // No text if after the last item;
    } else if (searchStart > -1 && searchStart < count && items[searchStart].start <= msTime
               && items[searchStart].end >= msTime) {
        // First see if this is the same as the last subtitle
        index = searchStart;
    } else if (searchStart > -1  && (searchStart + 1) < count && items[searchStart].end < msTime
               && items[searchStart + 1].start > msTime) {
        // No text if between the previous and next subtitle
    } else if (searchStart > -1  && (searchStart + 1) < count && items[searchStart + 1].start <= msTime
               && items[searchStart + 1].end >= msTime) {
        // See if this is the next subtitle
        index = searchStart + 1;
    } else {
        // Perform a full search from the beginning
        int i = 0;
        for (i = 0; i < count; i++) {
            if (items[i].start <= msTime && items[i].end >= msTime) {
                index = i;
                break;
            } else if (items[i].end > msTime) {
                break;
            }
        }
    }
    return index;
}
