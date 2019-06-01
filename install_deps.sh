#!/bin/bash

OSVER=`uname -a`

ARCHDEPS="libyaml ffmpeg-compat-57 libavresample python libsamplerate taglib \
          chromaprint-fftw python-six vamp-plugin-sdk intel-tbb zeromq capnproto"

UBUNTUDEPS="libyaml-dev ffmpeg libavresample-dev qt4-default python libsamplerate0 \
            libsamplerate0-dev libtag-extras-dev libchromaprint1 libchromaprint-dev \
            python-six vamp-plugin-sdk fftw3 libfftw3-bin libfftw3-dev libtbb2 \
            libtbb-dev capnproto libzmq3-dev libavformat-dev libavcodec-dev \
            libavcodec-extra"

if [ `echo ${OSVER} | grep "Ubuntu" | wc -l` -eq 1 ]
then
    echo "Using Ubuntu config"
    #sudo echo "deb http://download.opensuse.org/repositories/network:/messaging:/zeromq:/release-stable/Debian_9.0/ ./" >> /etc/apt/sources.list
    #sudo wget https://download.opensuse.org/repositories/network:/messaging:/zeromq:/release-stable/Debian_9.0/Release.key -O- | sudo apt-key add
    sudo apt install ${UBUNTUDEPS}
else
    echo "Using Arch Linux config"
    yay -S --needed ${ARCHDEPS}
fi



# cd cpp-netlib
# makepkg -si