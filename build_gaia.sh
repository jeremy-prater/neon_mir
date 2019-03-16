#!/bin/bash

cd gaia
./waf configure --with-asserts
./waf
sudo ./waf install
cd ..