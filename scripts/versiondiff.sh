#!/bin/bash

set -eu

catIfValid() {
    cat $1 2>/dev/null
}

printVersions() {
    VERSIONFILEPATH=$1
    if [ $(basename $VERSIONFILEPATH) = "versions" ]; then
        cat $VERSIONFILEPATH
    elif catIfValid $VERSIONFILEPATH/Shotcut.app/versions; then
        return 0
    elif catIfValid $VERSIONFILEPATH/Shotcut/Shotcut.app/versions; then
        return 0
    elif file $VERSIONFILEPATH | grep -q "compressed data"; then
        >&2 echo "Reading version from $VERSIONFILEPATH"...
        tar xOf $VERSIONFILEPATH Shotcut/Shotcut.app/versions
        if [ "$?" -ne "0" ]; then
            >&2 echo No versions file found in $VERSIONFILEPATH
            return 1
        fi
    else
        return 1
    fi
}

oldVersion=$(mktemp)
newVersion=$(mktemp)
trap "rm -f $oldVersion $newVersion" EXIT

printVersions $1 > $oldVersion
printVersions $2 > $newVersion

cd $(dirname $0)/../../

cat $oldVersion | while read dir sha; do
    pushd $dir > /dev/null

    oldsha=$sha
    newsha=$(egrep "^$dir" $newVersion | cut -f2 -d' ')
    if [ "$oldsha" = "$newsha" ]; then
        echo $dir: No new commits
        popd > /dev/null
        continue
    fi
    sharange=$oldsha..$newsha
    commitcount=$(git rev-list $sharange | wc -l)

    echo
    echo ---- $dir: $commitcount new commits
    git log --oneline $sharange

    popd > /dev/null
done
