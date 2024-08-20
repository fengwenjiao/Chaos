#!/bin/bash
if [ $# -lt 1 ]; then
    echo "usage: $0 bin [args..]"
    exit -1;
fi

echo "kill processes by name: $1"
pkill -u $USER -f $1

ps -aux | grep $1

# clean up
unset DMLC_PS_ROOT_URI
unset DMLC_PS_ROOT_PORT
unset DMLC_ROLE
unset START_TRAINER_NUM
unset DEBUG_OVERLAY
unset PS_VERBOSE
unset HEAPPROFILE