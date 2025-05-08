FROM satcomp-infrastructure:common
USER root
#  Install required softwares

RUN  sed -i s@/archive.ubuntu.com/@/mirrors.bfsu.edu.cn/@g /etc/apt/sources.list
RUN apt update
RUN DEBIAN_FRONTEND=noninteractive apt install -y vim cmake build-essential zlib1g-dev libopenmpi-dev wget unzip python3 gfortran curl
RUN apt install -y libboost-all-dev

WORKDIR /
COPY / /PRS


WORKDIR /PRS/third_party/kissat-inc
RUN ./configure --quiet
RUN make -j


WORKDIR /PRS/third_party/m4ri-20140914
RUN ./configure
RUN make -j

WORKDIR /PRS/third_party/yalsat
RUN ./configure.sh
RUN make -j

WORKDIR /PRS
RUN mkdir -p build
WORKDIR /PRS/build
RUN cmake ..
RUN make -j

WORKDIR /PRS