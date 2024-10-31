#!/bin/bash

function my_error()
{
    # maybe we want some logging functionality later?
    echo "${1}"
    exit 1
}

function my_success()
{
    if [ ! -z "${1}" ] ; then
	echo "${1}"
    fi
    exit 0
}

cnt_files=0

function my_find()
{
    a=`find "${1}"`

    if [ -z "${a}" ] ; then
	echo "<nothing found>"
    fi

    echo "------------------------------------"
    echo " File list in directory \"${1}\":"
    echo "------------------------------------"
    while read -r line ; do
	# we don't care for empty lines
	if [ -z "${line}" ] ; then continue; fi
	# we don't care about "."
	if [ "${line}" == "." ] ; then continue; fi
	# we don't care about empty dirs
	if [ ! -f "${line}" ]  ; then continue; fi
	# print file name - we can catch it in the caller
	echo " -> ${line}"
	(( cnt_files++ ))
    done <<< "${a}"
    echo "Total: ${cnt_files} file(s)"
}

cnt_grep=0

function my_grep()
{
    a=`grep -Hrn "${2}" "${1}"`
    cnt_grep=`echo "${a}" | wc -l`
    echo "------------------------------------"
    echo " Found ${cnt_grep} occurence(s) of \"${2}\""
    echo "------------------------------------"
    echo "${a}"
    echo "------------------------------------"
}

function my_summup()
{
    echo "The number of files are ${cnt_files} and the number of matching lines are ${cnt_grep}"
}

if [ "$#" != "2" ] ; then my_error "Not enough arguments" ; fi
if [ ! -d "${1}" ] ; then my_error "Directory \"${1}\" does not exist" ; fi

my_find "${1}"
my_grep "${1}" "${2}"
my_summup

my_success "Ok"
