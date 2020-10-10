FROM ubuntu:18.04
RUN apt-get update && apt-get install -y git cmake nasm vim libc6-dev-i386 bochs
