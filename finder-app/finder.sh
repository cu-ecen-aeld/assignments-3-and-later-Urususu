#!/bin/sh
# Script for assignment 1 and assignment 2
# Author: Urususu Shocku

if [ $# -lt 2 ]
then
    echo "One or more parameters missing. Please, input both arguments in the call to finder.sh"
    exit 1
else
    DIRECTORY=$1
    SEARCHSTR=$2
    
    if [ -d "$DIRECTORY" ]
    then
        file_list=$(find $DIRECTORY -type f)
        
        file_count=0
        matching_lines_count=0
 
        for file in $file_list; do
            file_count=$((file_count+1))
            matching_lines=$(grep -c "$SEARCHSTR" "$file")
            matching_lines_count=$((matching_lines_count + matching_lines))
		done
        echo "The number of files are $file_count and the number of matching lines are $matching_lines_count"
    else
        echo "The directory specified does not exist in the filesystem"
        exit 1
    fi	
fi

