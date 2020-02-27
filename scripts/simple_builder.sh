#!/bin/bash
set -e
INSTALL_PATH=tmp_install

if [[ ! -d build/ ]]; then
    mkdir -p "build/${INSTALL_PATH}"
    pushd build >/dev/null
    cmake .. \
	  -DCMAKE_INSTALL_PREFIX="${INSTALL_PATH}"
else
    pushd build >/dev/null
fi

time make -j8
time env CTEST_OUTPUT_ON_FAILURE=1 make test
make install DIR=${INSTALL_PATH}

popd > /dev/null
