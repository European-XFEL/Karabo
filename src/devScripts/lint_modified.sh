#!/bin/bash
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
#
# lints all modified C++ source files with clang-format and modified Python
# files with flake8 and isort.
#
# clang-format and isort modify the source files supplied to them. flake8
# only checks the input files for PEP8 violations. autopep8 is an option to
# have Python source files formatted in conformity to PEP8.
#

# Returns true if the first version argument is less than the second version argument
__version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }

__lint_clang_format() {
    # Put all locally modified files that conform to C++ source filename suffixes
    # and that are not Deleted or Renamed into the collection of files to be
    # checked for formatting.
    SRC_FILES=($(git diff --name-only --diff-filter=dr | grep -e "\.cc\'" -e "\.cpp\'"  -e "\.hh\'" -e "\.h\'" -e "\.hpp\'"))
    local -i num_files=${#SRC_FILES[@]}
    if [[ $num_files -eq 0 ]]; then
        echo "No file to clang-format"
        echo ""
        return 0  # no file to be formatted
    fi

    # Checks if clang-format is available and its version is at least the CI
    # version.
    CI_CLANG_FORMAT_VERSION="13.0.0"
    CLANG_FORMAT_VERSION=$(clang-format --version | grep -P '\d+\.\d+\.\d+' -o)
    echo "CLANG_FORMAT_VERSION = $CLANG_FORMAT_VERSION"
    ret_code=$?
    if [[ $ret_code != 0 ]]; then
        echo ""
        echo "WARNING: C++ format linting SKIPPED."
        echo "         Reason: clang-format not available. Please install clang-format version $CI_CLANG_FORMAT_VERSION."
        echo ""
        return
    fi
    if __version_lt $CLANG_FORMAT_VERSION $CI_CLANG_FORMAT_VERSION; then
        echo ""
        echo "WARNING: C++ format linting SKIPPED."
        echo "         Reason: Local clang-format version, $CLANG_FORMAT_VERSION, is older than the CI's, $CI_CLANG_FORMAT_VERSION."
        echo ""
        return
    fi

    echo ""
    echo "Running clang-format `clang-format --version` on modified C++ files ..."

    local -i formatted=0
    local -i errors=0
    local file_check_code
    for SRC_FILE in "${SRC_FILES[@]}"; do
        eval "clang-format -i $SRC_FILE"
        file_check_code=$?
        if [[ $file_check_code != 0 ]]; then
            ((errors+=1))
            echo "    ERROR clang-formatting file '$SRC_FILE' - error code: $file_check_code"
        else
            echo "    OK - clang-formatted file '$SRC_FILE'."
            ((formatted+=1))
        fi
    done
    echo "clang-format executed successfully on $formatted files out of $numfiles."
    echo ""
}

__lint_isort() {
    SRC_FILES=($(git diff --name-only --diff-filter=dr | grep -e "\.py\'"))
    local -i num_files=${#SRC_FILES[@]}
    if [[ $num_files -eq 0 ]]; then
        echo "No file to isort."
        echo ""
        return 0  # no file to be isorted
    fi

    ISORT_VERSION=$(isort --version)
    ret_code=$?
    if [[ $ret_code != 0 ]]; then
        echo ""
        echo "WARNING: isort based linting SKIPPED."
        echo "         Reason: isort not available."
        echo ""
        return
    fi

    echo ""
    echo "Running isort on modified Python files ..."

    local -i formatted=0
    local -i errors=0
    for SRC_FILE in "${SRC_FILES[@]}"; do
        eval "isort $SRC_FILE"
        file_check_code=$?
        if [[ $file_check_code != 0 ]]; then
            ((errors+=1))
            echo "    ERROR isorting file '$SRC_FILE' - error code: $file_check_code"
        else
            echo "    OK - isorted file '$SRC_FILE'"
            ((formatted+=1))
        fi
    done
    echo "isort executed successfully on $formatted files out of $num_files."
    echo ""
}

__lint_flake8() {
    SRC_FILES=($(git diff --name-only --diff-filter=dr | grep -e "\.py\'"))
    local -i num_files=${#SRC_FILES[@]}
    if [[ $num_files -eq 0 ]]; then
        echo "No file to flake."
        echo ""
        return 0  # no file to be formatted
    fi

    FLAKE_8_VERSION=$(flake8 --version)
    ret_code=$?
    if [[ $ret_code != 0 ]]; then
        echo ""
        echo "WARNING: flake8 based checking SKIPPED."
        echo "         Reason: flake8 not available."
        echo ""
        return
    fi

    echo ""
    echo "Running flake8 checks on modified Python files ..."

    local -i formatted=0
    local -i errors=0
    for SRC_FILE in "${SRC_FILES[@]}"; do
        eval "flake8 --tee $SRC_FILE"
        file_check_code=$?
        if [[ $file_check_code != 0 ]]; then
            ((errors+=1))
            echo "    ERROR flaking file '$SRC_FILE' - error code: $file_check_code"
        else
            echo "    OK - checked file '$SRC_FILE' for PEP8 violations"
            ((formatted+=1))
        fi
    done
    echo "flake8 executed successfully on $formatted files out of $num_files."
    echo ""
}


# ----- main -----
REPO_ROOT=`git rev-parse --show-toplevel`
pushd $REPO_ROOT > /dev/null

# Performs linting of the modified C++ files in the Framework
__lint_clang_format

# Performs linting of the modified Python files in the Framework
__lint_flake8
__lint_isort

popd
