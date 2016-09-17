// This is not meant to be used directly. It returns a function object
// to the QJSEngine environment.
//
// To build export-edl.js:
// npm install
// qml-browserify --globals false -o export-edl.js
// cat mlt2edl.js >> export-edl.js
// cat main.js >> export-edl.js
// See also rebuild.sh

(function main(xmlString, options) {
    var mltxml = new MltXmlParser(xmlString, options);
    return mltxml.createEdl();
})
