#!/bin/bash
full_path=$(readlink -f "${0}")
ROOT="$(dirname $(dirname ${full_path}))"
BUILDPATH=${ROOT}/build
INSTALLPATH=${BUILDPATH}/tmp
TARGET_INSTALL=/usr/local

test -d ${BUILDPATH} || mkdir ${BUILDPATH}
./scripts/build.sh -c
./scripts/build.sh -a
./scripts/build.sh -I

# Headers
echo -ne "Updating installed headers "
sudo cp -r ${INSTALLPATH}/include/urx ${TARGET_INSTALL}/include/.

# Library
echo -ne "... done.\nUpdating libraries "
test -L ${TARGET_INSTALL}/lib/x86_64-linux-gnu/liburx.so && sudo rm -f ${TARGET_INSTALL}/lib/x86_64-linux-gnu/liburx.so
sudo cp -r ${INSTALLPATH}/lib/liburx.so.1.0 ${TARGET_INSTALL}/lib/x86_64-linux-gnu/
sudo ln -s ${TARGET_INSTALL}/lib/x86_64-linux-gnu/liburx.so.1.0 ${TARGET_INSTALL}/lib/x86_64-linux-gnu/liburx.so

# scripts
echo -ne "... done.\nUpdating scripts "
sudo cp -r ${INSTALLPATH}/share/liburx ${TARGET_INSTALL}/share/
echo "... done."
