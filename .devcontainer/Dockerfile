FROM mcr.microsoft.com/vscode/devcontainers/base:0-buster

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends build-essential make git cmake ninja-build libprotobuf-dev protobuf-compiler libzmq3-dev