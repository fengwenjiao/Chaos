#!/bin/bash

trap 'cleanup' SIGINT

cleanup() {
    contcfg stop-server
    docker compose -f $DIR/docker-compose.yaml down
    exit 0
}

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# build constellation-dev
$DIR/../../docker/build-dev.sh

rm -rf $DIR/*.log

docker compose -f $DIR/docker-compose.yaml down

contcfg start-server 1000mbit 1000mbit 10 &

contcfg_pid=$!

# screen -dmS cons docker compose -f $DIR/docker-compose.yaml up --scale worker=6 --build > $DIR/output.log 2>&1
docker compose -f $DIR/docker-compose.yaml up --scale worker=6  > $DIR/output.log 2>&1 &

sleep 20

contcfg ctrl add-all modelsynctime-worker

sleep infinity
