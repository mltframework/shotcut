#!/bin/sh
xcrun notarytool submit --wait --keychain-profile notarytool "$1"
