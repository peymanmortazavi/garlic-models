FROM ubuntu:focal AS BASE

RUN apt update \
    && apt install make python3-pip -y
RUN pip3 install cmake conan
COPY . /project


# GCC 10 Target
FROM BASE AS GCC_10
RUN apt install gcc-10 g++-10 -y
RUN mkdir /output \
    && cd /output \
    && export CC=/usr/bin/gcc-10 && export CXX=/usr/bin/g++-10 \
    && conan profile new default --detect \
    && conan profile update settings.compiler.libcxx=libstdc++11 default \
    && conan install --build=missing -s build_type=Release /project \
    && cmake \
        -DCMAKE_BUILD_TYPE=Release /project
WORKDIR /output
