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

inotifywait -m ${dirs} -e modify -e create --exclude "\#" |
    while read path action file; do
	echo "path: ${path} action: ${action} file: ${file}"
	pushd build > /dev/null
	case "${action}" in
	    MODIFY)
		case ${file} in
		    CMakeLists.txt)
			echo "${path}/${file} updated, trigger full rebuild"
			cmake .. -DBUILD_TESTS=ON
			make clean && make -j8 && make test
			;;
		    *)
			time make -j8 && time make test
			;;
		esac
		;;
	    CREATE)
		case ${file} in
		    CMakeLists.txt)
			echo "[C] ${path}/${file} created, trigger full rebuild"
			cmake .. -DBUILD_TESTS=ON && make clean && time make -j8 && time make test
			;;
		    *)
			time make -j8 && time make test
			;;
		esac
		echo "{file} - something else"
		;;
	esac
	popd > /dev/null
    done
popd > /dev/null
