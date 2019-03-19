#!/bin/bash

cd gaia
python2 ./waf configure --with-asserts --with-tests --prefix=/usr mode=debug
python2 ./waf
sudo python2 ./waf install
cd ..