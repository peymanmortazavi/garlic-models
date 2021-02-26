FROM gcc:10

RUN apt update && apt install python3-pip -y
RUN pip3 install cmake conan
COPY . /project
RUN mkdir /output \
    && cd /output \
    && conan profile new default --detect \
    && conan profile update settings.compiler.libcxx=libstdc++11 default \
    && conan install --build=missing -s build_type=Release /project \
    && cmake -DCMAKE_BUILD_TYPE=Release /project
WORKDIR /output
