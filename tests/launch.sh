#!/bin/bash

trap 'cleanup' SIGINT

cleanup() {
    # clean up
    unset DMLC_PS_ROOT_URI
    unset DMLC_PS_ROOT_PORT
    unset DMLC_ROLE
    unset START_TRAINER_NUM
    unset DEBUG_OVERLAY
    unset PS_VERBOSE
    unset HEAPPROFILE
    kill -9 ${scheduler_pid}
    for pid in ${pids[@]}; do
        kill -9 ${pid}
    done

    exit 0
}

# set -x
if [ $# -lt 2 ]; then
    echo "usage: $0 num_trainers bin [args..]"
    exit -1
fi

TRAINER_NUM=$1
shift
bin=$1
shift
arg="$@"

# check num_trainers
if [ $TRAINER_NUM -lt 1 ]; then
    echo "num_trainers should be at least 1"
    exit -1
fi

# start the scheduler
export DMLC_PS_ROOT_URI='127.0.0.1'
export DMLC_PS_ROOT_PORT=8000
export DMLC_ROLE='scheduler'
export START_TRAINER_NUM=$TRAINER_NUM

export DEBUG_OVERLAY=1
export PS_VERBOSE=1
"${bin}" "${arg}" &

# record the  pid
scheduler_pid=$!
# start trainers
export DMLC_ROLE='trainer'
for ((i = 0; i < ${TRAINER_NUM}; ++i)); do
    export HEAPPROFILE=./W${i}
    "${bin}" "${arg}" &
    pids[${i}]=$!
done

wait
