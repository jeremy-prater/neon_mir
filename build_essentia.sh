#!/bin/bash

cd essentia
./waf configure --build-static mode=debug --with-gaia --with-cpptests --with-examples
./waf
sudo ./waf install
cd ..