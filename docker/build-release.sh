#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=$DIR/..
BUILD_DIR=$ROOT_DIR/build

# ensure that the base image is built
BASE_TAG=$($DIR/build-base.sh | grep "constellation-base:" | cut -d':' -f2)

# insert the base tag into the Dockerfile
sed -i "s/FROM constellation-base.*/FROM constellation-base:$BASE_TAG/" $DIR/dockerfile

# build the release image
docker build $ROOT_DIR -f $DIR/dockerfile  -t "constellation:${BASE_TAG}"

# remove the base tag from the Dockerfile
sed -i "s/FROM constellation-base:.*/FROM constellation-base/" $DIR/dockerfile