#!/bin/sh
qml-browserify --globals false -o export-edl.js
cat mlt2edl.js >> export-edl.js
cat main.js >> export-edl.js
