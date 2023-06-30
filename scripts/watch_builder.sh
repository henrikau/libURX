#!/bin/bash
# require: inotify-tools

# Drop full debug output if tests fails
export CTEST_OUTPUT_ON_FAILURE=1

# find directorires with files
pushd "$(dirname `realpath $0`)/../" > /dev/null

dirs=$(for f in $(find . -name "*.[c|h]pp" -o -name "*.h" -o -name "CMakeLists.txt"); do dirname ${f}; done|grep -v build|sort|uniq)

echo "watching directories:  ${dirs}"

if [[ -z ${dirs} ]]; then
    echo "Could not find any source-files, try to move to the root of the project!"
    exit 1
fi

while inotifywait -e modify -e create --exclude "\#" ${dirs} ; do
    ./scripts/build.sh -f -t -p
    # do a refresh-scan of tracked files
    dirs=$(for f in $(find . -name "*.[c|h][pp]?" -o -name "CMakeLists.txt"); do dirname ${f}; done|grep -v build|sort|uniq|tr '\n' ' ')
    if [[ -z ${dirs} ]]; then
	echo "Could not find any source-files, try to move to the root of the project!"
	exit 1
    fi
    echo "Updating TAGS..."
    find . -name "*" -type f -print | etags  -
done
