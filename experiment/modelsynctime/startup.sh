#!/bin/bash

trap 'cleanup' SIGINT

cleanup() {
    contcfg --socket-path ~/contcfg-xrx.sock stop-server 
    docker compose -f $DIR/docker-compose.yaml down
    exit 0
}

# get the directory of the script
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# load configuration
source $DIR/expconfig.env

# build constellation-dev
# $DIR/../../docker/build-dev.sh

# clear logs and stop previous containers
rm -rf $DIR/*.log
docker compose -f $DIR/docker-compose.yaml down

# start contcfg to set the bandwidth
contcfg --socket-path ~/contcfg-xrx.sock start-server $MIN_MBPS $MAX_MBPS 10 &
contcfg_pid=$!

# launch the experiment
docker compose -f $DIR/docker-compose.yaml up --scale worker=$STARTUP_WORKERS  > $DIR/output.log 2>&1 &
sleep 20
# set bandwidth for the workers
contcfg --socket-path ~/contcfg-xrx.sock ctrl add-all modelsynctime-worker 

sleep infinity
