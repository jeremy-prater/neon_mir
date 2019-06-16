#!/bin/bash

cp -v ./extensions/boostbuffer/* ./essentia/src/essentia/streaming/algorithms/

cd essentia
./waf configure mode=debug --with-gaia --prefix=/usr --mode=debug
./waf
sudo ./waf install
cd ..