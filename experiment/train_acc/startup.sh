#!/bin/bash

# get the directory of the script
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# load configuration
if [ -f "$DIR/expconfig.env" ]; then
    source "$DIR/expconfig.env"
else
    echo "Configuration file  $DIR/expconfig.env not found"
    exit 1
fi

PROJECT_NAME="${MODEL_NAME}-${STARTUP_WORKERS}-${START_TRAINER_NUM}"

# create directory for the output
mkdir -p $DIR/$PROJECT_NAME
# rm -rf $DIR/$PROJECT_NAME/*.log

trap 'cleanup' SIGINT
cleanup() {
    docker compose -p "$PROJECT_NAME" -f "$DIR/docker-compose.yaml" down
    exit 0
}


# launch the experiment
docker compose -p $PROJECT_NAME -f $DIR/docker-compose.yaml up --scale worker=$STARTUP_WORKERS  > $DIR/$PROJECT_NAME/output.log 2>&1 &

sleep infinity