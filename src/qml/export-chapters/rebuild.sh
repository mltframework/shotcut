#!/bin/sh
qml-browserify --globals false -o export-chapters.js
cat mlt2chapters.js >> export-chapters.js
cat main.js >> export-chapters.js
