#!/bin/bash
# Script for assignment 1 and assignment 2
# Author: Urususu Shocku

if [ $# -lt 2 ]
then
    echo "One or more parameters missing. Please, input both arguments in the call to writer.sh"
    exit 1
else
    PATHTOFILE=$1
    WRITESTR=$2

    echo $WRITESTR > $PATHTOFILE

    if [ -e $PATHTOFILE ]
    then
        exit 0
    else
        echo "File can not be created"
        exit 1
    fi
fi

