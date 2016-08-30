// Use this to test with node.js during development:
// npm install
// node test-node.js /path/to/some.mlt

var mlt2edl = require('./mlt2edl.js');

var fs = require('fs');
var process = require('process');

var fileName = process.argv[2];
var xmlString = fs.readFileSync(fileName, 'utf8');
var mltxml = new mlt2edl.MltXmlParser(xmlString);
var edlString = mltxml.createEdl();
console.log(edlString);
