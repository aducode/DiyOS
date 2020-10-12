#FROM ubuntu:18.04
# FROM dorowu/ubuntu-desktop-lxde-vnc:bionic
FROM dorowu/ubuntu-desktop-lxde-vnc:focal
RUN sed -i 's#mirror://mirrors.ubuntu.com/mirrors.txt#https://mirrors.aliyun.com/ubuntu/#' /etc/apt/sources.list
RUN apt update && apt install -y git cmake nasm libc6-dev-i386 build-essential xorg-dev
WORKDIR /root/workspace
RUN wget -q  http://bochs.sourceforge.net/svn-snapshot/bochs-20201008.tar.gz
RUN tar zxvf bochs-20201008.tar.gz
WORKDIR /root/workspace/bochs-20201008
RUN ./configure --enable-debugger --enable-disasm && make && make install
RUN ln -snf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime && echo "Asia/Shanghai" > /etc/timezone && apt-get install -y tzdata
ENTRYPOINT ["/bin/bash"]
