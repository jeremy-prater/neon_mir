#!/bin/bash

./install_deps.sh

rm -rf gaia
rm -rf essentia

git submodule init
git submodule update

./build_gaia.sh
./build_essentia.sh