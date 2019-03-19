#!/bin/bash

cd essentia
./waf configure --build-static mode=debug --with-gaia --with-cpptests --with-examples --prefix=/usr mode=debug
./waf
sudo ./waf install
cd ..