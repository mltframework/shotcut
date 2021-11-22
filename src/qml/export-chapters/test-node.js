// Use this to test with node.js during development:
// npm install
// node test-node.js /path/to/some.mlt

var mlt2chapters = require('./mlt2chapters.js');

var fs = require('fs');
var process = require('process');

var fileName = process.argv[2];
var xmlString = fs.readFileSync(fileName, 'utf8');
var mltxml = new mlt2chapters.MltXmlParser(xmlString);
var string = mltxml.createChapters();
console.log(string);
