#!/bin/bash

cd essentia
./waf configure mode=debug --with-gaia --prefix=/usr mode=debug
./waf
sudo ./waf install
cd ..