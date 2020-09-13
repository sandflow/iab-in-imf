FROM ubuntu:bionic

RUN apt-get update

# install dependencies

RUN apt-get -y install build-essential

RUN apt-get -y install git

RUN apt-get -y install cmake

RUN apt-get -y install libboost-all-dev

RUN apt-get -y install libxerces-c-dev

RUN apt-get -y install  libssl-dev

# build ASDCPLib and iab-in-imf toolkit

WORKDIR /usr/src/iab-in-imf

COPY . .

RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install && ldconfig