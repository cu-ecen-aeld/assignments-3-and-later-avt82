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

if [ "$#" != "2" ] ; then my_error "Not enough arguments" ; fi
if [ -d "${1}" ] ; then my_error "${1} is a directory" ; fi

dir=`dirname "${1}"`
if [ ! -d "${dir}" ] ; then
    echo "Creating directory \"${dir}\"..."
    mkdir -p "${dir}"
fi
echo "Writing file \""`basename "${1}"`"\"..."
echo "${2}" > ${1}

my_success
