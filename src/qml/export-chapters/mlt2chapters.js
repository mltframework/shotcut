/*
 * MltXmlParser class Copyright (c) 2021-2025 Meltytech, LLC
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

var xmldoc;

if (typeof module !== 'undefined' && module.exports) {
    // We're being used in a Node-like environment
    xmldoc = require('xmldoc');
} else {
    // assume it's attached through qml-browserify
    xmldoc = modules.xmldoc;
    if (!xmldoc)
        throw new Error("Expected xmldoc to be defined. Make sure you're including xmldoc.js before this file.");
}

////////////////////////////////////////////////////////////////////////////////

function MltXmlParser(xmlString, options) {
    let self = Object.create(this);
    self.xmldoc = new xmldoc.XmlDocument(xmlString);
    self.includeRanges = self.get(options, 'includeRanges', false);
    self.selectedColors = self.get(options, 'colors', []);
    self.ffmetadataFormat = self.get(options, 'ffmetadata', false);
    return self;
}

MltXmlParser.prototype.get = function(dict, name, defaultValue) {
    return (typeof dict === 'object' && name in dict)? dict[name] : defaultValue;
};

MltXmlParser.prototype.timecode = function(value) {
    if (typeof value === 'string') {
        // Determine if this is a MLT "clock" time string.
        if (value[8] === '.' || value[8] === ',' || value[8] === ':' || value[8] === ';') {
                if (value.substring(0,3) === '00:') {
                    return value.substring(3, 8);
                } else {
                    return value.substring(0, 8);
            }
        }
        return value;
    }
    return 'timecode error: ' + value + '.';
};

MltXmlParser.prototype.createChapters = function() {
    let self = this;
    let chaptersStr = self.ffmetadataFormat ? ";FFMETADATA1\n" : "00:00 Intro\n";
    let markers = [];

    this.xmldoc.childrenNamed('tractor').forEach(function (tractor) {
        tractor.childrenNamed('properties').forEach(function (p) {
            if (p.attr.name === 'shotcut:markers') {
                p.childrenNamed('properties').forEach(function (m) {
                    let marker = {};
                    m.childrenNamed('property').forEach(function (prop) {
                        if (prop.attr.name === 'start') {
                            marker.start = prop.val;
                            marker.timecode = self.timecode(prop.val);
                            marker.seconds = 3600 * parseInt(prop.val.substring(0, 2)) + 60 * parseInt(prop.val.substring(3, 5)) + parseInt(prop.val.substring(6, 8));
                            if (marker.timecode === '00:00' && !self.ffmetadataFormat) {
                                chaptersStr = '';
                           }
                        } else if (prop.attr.name === 'end') {
                            marker.end = prop.val;
                        } else if (prop.attr.name === 'text') {
                            marker.text = prop.val;
                        } else if (prop.attr.name === 'color') {
                            marker.color = prop.val;
                        }
                    });
                    if ((self.includeRanges || marker.end === marker.start)
                            && (self.selectedColors.length === 0 || self.selectedColors.includes(marker.color))) {
                        if (markers.length > 0) {
                            markers[markers.length - 1].nextMarker = marker;
                        }
                        markers.push(marker);
                    }
                });
                return;
            }
        });
    });
    markers.sort(function compare(a, b) {
        return (a.seconds === b.seconds)? 0 : (a.seconds < b.seconds)? -1 : 1;
    });
    markers.forEach(function (marker) {
        if (self.ffmetadataFormat) {
            let end = (marker.end === marker.start && marker.nextMarker) ? (marker.nextMarker.seconds - 1) : (marker.endSeconds + 1);
            chaptersStr += "[CHAPTER]\nTIMEBASE=1/1\nSTART=" + marker.seconds + "\nEND=" + end + "\ntitle=" + marker.text + "\n";
        } else {
            chaptersStr += marker.timecode + ' ' + marker.text + "\n";
        }
    });
    return chaptersStr;
};


////////////////////////////////////////////////////////////////////////////////
// Are we being used in a Node-like environment?
if (typeof module !== 'undefined' && module.exports)
    module.exports.MltXmlParser = MltXmlParser;
