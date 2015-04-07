#!/usr/bin/env bash

set -e
cd $(dirname $0)

updateSummary=$(mktemp)
trap "rm $updateSummary" EXIT
echo Pulled in new commits from dependency repoes >> $updateSummary
echo >> $updateSummary

die() {
    echo "ERROR: $@"
    exit 1
}

wasSetOnlyOnce() {
    local VARNAME=$1
    local FILE=$2
    test $(egrep "^${VARNAME}=" $FILE | wc -l) -eq 1
}

readBashVar() {
    local NAME=$1
    local FILE=$2

    if ! wasSetOnlyOnce ${NAME} $FILE; then
        die Expected ${NAME} to be set exactly once in $FILE
    fi
    egrep -o "^${NAME}=[^ ]*" $FILE | cut -d= -f2
}

chdirToMatchingRepo() {
    remote=$1
    for repo in ../../*; do
        if test -f $repo/.git/config && grep -q $remote $repo/.git/config; then
            cd $repo
            return 0
        fi
    done
    return -1
}

setBashVar() {
    local NAME=$1
    local VALUE=$2
    local FILE=$3
    local COMMENT=$4
    sed -i "s?^${NAME}=.*?${NAME}=${VALUE} #${COMMENT}?g" $FILE
}

findRemote() {
    local VARNAME=$1
    local FILE=$2
    if [ $(egrep "^${VARNAME}=" $FILE | wc -l) -ne 1 ]; then
        die Expected $VARNAME to be set only once in $FILE
    fi

    grep "${VARNAME}.*FETCHFROM" $FILE | egrep -o 'FETCHFROM:[^ ]+' | cut -d: -f2
}

findRepository() {
    local INDEX=$1
    local FILE=$2
    egrep -o "REPOLOCS\[$INDEX\]=.*" $FILE | cut -d= -f2 | sed 's/"//g'
}

validate() {
    local NAME=$1
    local FILE=$2

    if ! wasSetOnlyOnce ${NAME}_REVISION $FILE; then
        die Expected ${NAME}_REVISION to be set exactly once in $FILE
    fi
    if ! wasSetOnlyOnce ${NAME}_HEAD $FILE; then
        die Expected ${NAME}_HEAD to be set exactly once in $FILE
    fi
    if [ -z "$(readBashVar ${NAME}_REVISION $FILE)" ]; then
        die ${NAME}_REVISION was not set
    fi
}

updateRevision() {
    local INDEX=$1
    local NAME=$2

    if ! wasSetOnlyOnce ${NAME}_HEAD build-shotcut.sh; then
        die ${NAME}_HEAD was not set
    fi

    local HEAD=$(readBashVar ${NAME}_HEAD build-shotcut.sh)

    if [ "$HEAD" = "1" ]; then
        echo -- $NAME Reading from HEAD
        validate $NAME build-shotcut.sh
        REMOTE=$(findRemote ${NAME}_REVISION build-shotcut.sh)
        if [ -z "$REMOTE" ]; then
            die $NAME was set to read from HEAD, but did not have a FETCHFROM tag
        fi
        echo Remote detected: $REMOTE
        REPO=$(findRepository $INDEX build-shotcut.sh)
        echo Repository detected:  $REPO
        PREVIOUS_SHA=$(readBashVar ${NAME}_REVISION build-shotcut.sh)
        if ! chdirToMatchingRepo $REPO; then
            die Failed to find repo containing $REPO
        fi
        echo Entered $(pwd)
        git fetch

        NEWSHA=$(git rev-parse $REMOTE)
        DESCRIPTION=$(git describe --tags $NEWSHA 2>/dev/null || echo unknown-tag)
        echo $REMOTE was resolved to $NEWSHA \($DESCRIPTION\)
        if [ "$NEWSHA" != "$PREVIOUS_SHA" ]; then
            echo $NAME: moving to $DESCRIPTION \($(git log --oneline ${PREVIOUS_SHA}..${NEWSHA} | wc -l) new commits\) | tee -a $updateSummary
            echo "    $PREVIOUS_SHA..$NEWSHA" | tee -a $updateSummary
            echo Inspect with: git -C $(pwd) log $PREVIOUS_SHA..$NEWSHA
        fi
        cd - > /dev/null
        setBashVar ${NAME}_REVISION "$NEWSHA" "build-shotcut.sh" "$DESCRIPTION FETCHFROM:${REMOTE}"

    elif [ "$HEAD" = "0" ]; then
        echo -- $NAME Not reading from HEAD
    else
        die ${NAME}_HEAD is invalid, set to $HEAD
    fi
}

updateRevision 0  FFMPEG
updateRevision 1  MLT
updateRevision 2  FREI0R
updateRevision 3  X264
updateRevision 4  LIBVPX
updateRevision 5  MOVIT
updateRevision 7  SHOTCUT
updateRevision 9  WEBVFX
updateRevision 10 VIDSTAB
updateRevision 11 LIBEPOXY
updateRevision 12 LIBOPUS
updateRevision 13 X265

if [ $(cat $updateSummary | wc -l) -gt 2 ]; then
    git commit -F $updateSummary build-shotcut.sh
fi

