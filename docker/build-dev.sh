#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=$DIR/..
BUILD_DIR=$ROOT_DIR/build

# ensure that the base image is built
$DIR/build-base.sh

cmake -B $BUILD_DIR -S $ROOT_DIR
cmake --build $BUILD_DIR

# build the dev image
docker build $ROOT_DIR -f $DIR/dockerfile-dev  -t constellation-dev