FROM pytorch/pytorch:2.2.2-cuda11.8-cudnn8-runtime

# cmake build tools
RUN env | grep -i _PROXY
RUN apt-get update 
RUN apt-get install -y --no-install-recommends cmake gcc g++ make libc6-dev ninja-build libprotobuf-dev protobuf-compiler libzmq3-dev

# copy the current directory contents into the container at /constellation, excluding the files in .dockerignore
COPY . /constellation

# set the working directory to /constellation
WORKDIR /constellation

# build the project
RUN mkdir build && cd build && cmake .. && cmake --build .

# install the python package
WORKDIR /constellation/python
RUN pip3 install -r requirements-dev.txt && pip3 install .

RUN apt-get autoremove -y && apt-get clean && rm -rf /var/lib/apt/lists/*

# fix for libstdc++ issue. Optional, but may be necessary for some systems
RUN mv /opt/conda/lib/libstdc++.so.6 /opt/conda/lib/libstdc++.so.6.bak
ENV LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

