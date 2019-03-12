#!/bin/sh

if [ "$1" = "" ]; then
    LOC_BUILD=`svnversion -c $FRCDIR | cut -f2 -d":"`
    SDK_BUILD=`svnversion -c $OCTEON_ROOT| cut -f2 -d":"`

    #echo "SINGLE_BUILD=$SINGLE_BUILD, SDK_BUILD=$SDK_BUILD"
   
    if [ "`echo $LOC_BUILD |grep M`" = "$LOC_BUILD" ]; then
        echo "Some modified in $FRCDIR, Can't not be release!"
#        exit 1
    fi

    if [ "`echo $SDK_BUILD |grep M`" = "$SDK_BUILD" ]; then
        echo "Some modified in $OCTEON_ROOT, Can't not be release!"
        exit 1
    fi

    if [ $SDK_BUILD -gt $LOC_BUILD ]; then
        BUILD_VERSION=$SDK_BUILD
    else
        BUILD_VERSION=$LOC_BUILD
    fi
else
    BUILD_VERSION=`date +6%H%M`
fi

echo $BUILD_VERSION
