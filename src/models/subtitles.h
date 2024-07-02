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

#ifndef SUBTITLES_H
#define SUBTITLES_H

#include <string>
#include <vector>

namespace Subtitles {

struct SubtitleItem {
    int64_t start;
    int64_t end;
    std::string text;
};

typedef std::vector<Subtitles::SubtitleItem> SubtitleVector;

SubtitleVector readFromSrtFile(const std::string &path);
bool writeToSrtFile(const std::string &path, const SubtitleVector &items);
SubtitleVector readFromSrtString(const std::string &text);
bool writeToSrtString(std::string &text, const SubtitleVector &items);
int indexForTime(const SubtitleVector &items, int64_t msTime, int searchStart);
}

#endif // SUBTITLES_H
