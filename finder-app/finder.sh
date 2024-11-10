#!/bin/sh

if [ "$#" != "2" ] ; then
    echo "Not enough arguments"
    exit 1
fi

if [ ! -d "${1}" ] ; then
    echo "Directory \"${1}\" does not exist" ; 
    exit 1
fi

# my_find $1
cnt_files=`find "${1}" | wc -l`
if [ "${cnt_files}" != 0 ] ; then
    $(( cnt_files = cnt_files - 1 )) 2>/dev/null
fi

cnt_grep=0
a=`grep -Hrn "${2}" "${1}" `
cnt_grep=`echo "${a}" | wc -l`
echo "------------------------------------"
echo " Found ${cnt_grep} occurence(s) of \"${2}\""
echo "------------------------------------"
echo "${a}"
echo "------------------------------------"

echo "The number of files are ${cnt_files} and the number of matching lines are ${cnt_grep}"

exit 0
