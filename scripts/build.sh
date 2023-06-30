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
    -I run INSTALL target
    -p Run pmccabe (cyclomatic qualtiy metric)
EOF
}
export CTEST_OUTPUT_ON_FAILURE=1

full_path=$(readlink -f "${0}")
ROOT="$(dirname $(dirname ${full_path}))"
BUILDPATH=${ROOT}/build
INSTALLPATH=${BUILDPATH}/tmp

test -d ${BUILDPATH} || mkdir ${BUILDPATH}
pushd "${BUILDPATH}"

while getopts "acCftIp" o; do
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
	'I')
	    DO_INSTALL=1
	    ;;
	't')
	    DO_TEST=1
	    ;;
	'p')
	    DO_PMCCABE=1
	    ;;
	*)
	  usage
	  exit 1
	    ;;
	esac
done

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
#export CC=/usr/bin/gcc
#export CXX=/usr/bin/g++
do_cmake ()
{
    pushd "${BUILDPATH}" > /dev/null
    cmake .. \
	  -DCMAKE_INSTALL_PREFIX="${INSTALLPATH}" \
	  -DCMAKE_INSTALL_INCLUDEDIR="${INSTALLPATH}" \
	  -DCMAKE_INSTALL_LIBDIR=lib \
	  -DALT_LIBS="external/usr" \
	  -DEXAMPLES=ON

    popd > /dev/null
}

do_pmccabe ()
{
    if [[ -z $(which pmccabe) ]]; then
	echo "Cannot find cyclomatic complexity, missing pmccabe. Please install."
	exit 1;
    fi

    pushd ${ROOT}/src > /dev/null
    pmccabe -v /dev/null | head -n6;
    pmccabe *.c* | sort -nk 2 | tail -n20

    echo "Files: ";
    pmccabe -F *.c* |sort -nk 2
    popd > /dev/null
}

test -z ${DO_CMAKE} || do_cmake
test -z ${DO_CLEAN} || make clean
test -z ${DO_MAKE} || time make -j$(nproc)
test -z ${DO_TEST} || make test
test -z ${DO_PMCCABE} || do_pmccabe
test -z ${DO_INSTALL} || make install


popd > /dev/null 		# build
