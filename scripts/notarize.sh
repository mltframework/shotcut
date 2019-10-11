#!/bin/sh
xcrun altool --notarize-app --primary-bundle-id "$(basename $1)" --username "dan@dennedy.org" --password "@keychain:altool" --file "$1"

