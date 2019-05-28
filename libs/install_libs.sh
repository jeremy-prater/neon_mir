#!/bin/bash

CMAKE_EXTRA="-DCMAKE_INSTALL_PREFIX:PATH=/usr"

LIBS="debuglogger"

for lib in ${LIBS}
do
    echo "==> Building [${lib}] <=="
    pushd ${lib}
        
        # TODO : Once stable, stop trashing builds
        rm -rf build

        mkdir -p build
        cd build
        cmake ${CMAKE_EXTRA} ..
        make -j 8
        sudo make install
    popd
done