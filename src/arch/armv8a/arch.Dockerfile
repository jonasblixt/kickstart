FROM ubuntu:xenial

ENV KS_ARCH=armv8a
ENV HOSTNAME=ks-dev
ENV PS1="\h:\w\$ "

RUN echo "deb mirror://mirrors.ubuntu.com/mirrors.txt xenial main restricted universe multiverse" > /etc/apt/sources.list && \
    echo "deb mirror://mirrors.ubuntu.com/mirrors.txt xenial-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
    echo "deb mirror://mirrors.ubuntu.com/mirrors.txt xenial-security main restricted universe multiverse" >> /etc/apt/sources.list && \
    DEBIAN_FRONTEND=noninteractive apt-get update && \
    apt-get install -y \
    apt-transport-https \
    tar \
    xz-utils \
    curl \
    sudo \
    automake \
    autopoint \
    gcc-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    libtool \
    git \
    cpio \
    pkg-config \
    bison

RUN apt-get install -y \
    gettext

RUN echo 'export PS1="ks-dev \W $ "' > /bashrc

CMD ["bash","--rcfile", "/bashrc"]
