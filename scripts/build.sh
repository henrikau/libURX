#!/bin/bash
#
# expect to be called relative to scripts, i.e.
#     ./scripts/build.sh
#
set -e
usage ()
{
echo "Usage: $0 [OPTION]" 1>&2;
cat <<EOF
    -a all (force cmake to run), default option
    -c clean
    -f (fast-)compile
    -t run tests
EOF
}
export CTEST_OUTPUT_ON_FAILURE=1

pushd "$(dirname $(realpath $0))/../"

test -d build || mkdir build/
pushd build/ > /dev/null

while getopts "acCft" o; do
    case "${o}" in
	'a')
	    DO_CMAKE=1
	    DO_MAKE=1
	    DO_TEST=1
	    ;;
	'c')
	    DO_CLEAN=1
	    ;;
	'C')
	    DO_CMAKE=1
	    ;;
	'f')
	    DO_MAKE=1
	    ;;
	'h')
	    usage
	    exit 0
	    ;;
	't')
	    DO_TEST=1
	    ;;
	*)
	  usage
	  exit 1
	    ;;
	esac
done

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
# export CC=/usr/bin/gcc
# export CXX=/usr/bin/g++

test -z ${DO_CMAKE} || cmake .. -DALT_LIBS="external/usr"
test -z ${DO_CLEAN} || make clean
test -z ${DO_MAKE} || time make -j$(nproc)
test -z ${DO_TEST} || make test


popd > /dev/null 		# build
popd > /dev/null # realpath...
