#!/bin/sh

case "$1" in
    "-start"|"start")
    start-stop-daemon --start --name aesdsocket --startas /usr/bin/aesdsocket -- -d
    exit $?
    ;;
    "-stop"|"stop")
    start-stop-daemon --stop --name aesdsocket
    exit $?
    ;;
    "-restart"|"restart")
    "$0" stop
    "$0" start
    exit $?
    ;;
    *)
    echo "Action hasn't been specified"
    exit 1
    ;;
esac
