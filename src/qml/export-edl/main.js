// This is not meant to be used directly. It returns a function object
// to the QJSEngine environment.
//
// To build export-edl.js:
// npm install
// qml-browserify --globals false -o export-edl.js
// cat mlt2edl.js >? export-edl.js
// cat main.js >> export-edl.js
// And then comment out the 3rd line that begins with "var previousRequire".

(function main(xmlString) {
    var mltxml = new MltXmlParser(xmlString);
    return mltxml.createEdl();
})
