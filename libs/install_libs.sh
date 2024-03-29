#!/bin/bash

CMAKE_EXTRA="-DCMAKE_INSTALL_PREFIX:PATH=/usr"

LIBS="debuglogger pulse-audio-input scene-manager"

for lib in ${LIBS}
do
    echo "==> Building [${lib}] <=="
    pushd ${lib}
        
        # TODO : Once stable, stop trashing builds
        rm -rf build

        mkdir -p build
        cd build
        cmake ${CMAKE_EXTRA} ..
        make -j 16
        sudo make install
    popd
done