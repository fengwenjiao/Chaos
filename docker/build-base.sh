#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# extract the base image name from the Dockerfile
PYTORCH_BASE_IMAGE=$(grep FROM $DIR/dockerfile-base | awk '{print $2}')
# extract the base image tag
PYTORCH_BASE_TAG=$(echo $PYTORCH_BASE_IMAGE | awk -F: '{print $2}')
# combine pytorch- and the tag
BASE_IMAGE_TAG=pytorch-$PYTORCH_BASE_TAG
BASE_IMAGE=constellation-base:$BASE_IMAGE_TAG
echo "$BASE_IMAGE"

docker build $DIR -f $DIR/dockerfile-base  -t $BASE_IMAGE
