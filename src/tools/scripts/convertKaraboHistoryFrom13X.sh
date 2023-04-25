#! /bin/bash
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

if [ $# -lt 1 ] || [ $1 = "-h" ] || [ $1 = "--help" ]; then
    echo
    # Cut off everything before (and including) the last '/' in $0:
    echo "${0##*/} [-h | --help] karaboHistory [karaboHistory2 [...]]"
    echo
    echo "Convert one or more karaboHistory directories of Karabo's"
    echo "DataLogger from the format used in release 1.3.X to the format"
    echo "used since 1.4.0."
    echo "It has to be run while the data logging is stopped."
    echo
    echo "Options: -h | --help: This help"
    echo
    echo "The conversion (i.e. renaming files) is done in place and there"
    echo "is no tool to convert back. Therefore it is recommended to make a"
    echo "backup before applying this command, e.g. by running"
    echo
    echo "tar czf karaboHistory.tgz karaboHistory"
    echo
    if [ $# -lt 1 ]; then
        exit 1
    else
        exit 0
    fi
fi

# Set option so that file name expansion does not expand to the pattern
# itself if no match is found:
shopt -s nullglob

# integer counters
declare -i numErrors=0
declare -i numDirectories=0

# Keep track of where we started
startDir=$(pwd)

# Loop on all arguments.
for historyDir ; do
    cd $startDir
    echo "Start converting directory '$historyDir'"

    if ! cd $historyDir ; then
        echo "  ERROR: Cannot cd to '$historyDir': skip!" >&2
        ((numErrors++))
        continue
    fi
    ((numDirectories++))

    # Loop on subdirectories that should be deviceIds.
    declare -i countDevices=0
    for deviceName in *; do
        echo "  Start treating device '$deviceName'."
        if ! cd $deviceName ; then
            echo "    ERROR: Cannot cd to device directory '$deviceName': skip!" >&2
            ((numErrors++))
            continue
        fi
        ((countDevices++))

        # Treat directory idx that possibly contains index files.
        if cd idx ; then
            declare -i counter=0
            prefixToRemove=${deviceName}_configuration_
            for oldIndexFile in ${prefixToRemove}*-*-index.bin ; do
                # New file name: Add new prefix after stripping old prefix.
                newIndexFile=archive_${oldIndexFile#$prefixToRemove}
                command="mv $oldIndexFile $newIndexFile"
                if ! $command ; then
                    echo "    ERROR: '$command' failed in $(pwd)." >&2
                    ((numErrors++))
                else
                    ((counter++))
                fi
            done
            echo "    Converted $counter index files for device '${deviceName}'."
            # Go back from idx to device directory:
            cd ../
        else
            # The 'idx' directory should be there, but it does not harm if not.
            echo "    WARNING: Cannot cd to 'idx' inside '$deviceName' - continue with 'raw'."
        fi
        if ! cd raw  ; then
            echo "    ERROR: Cannot cd to 'raw' inside '$deviceName': skip!" >&2
            ((numErrors++))
            # Go back to be able to cd to next device directory.
            cd ../
            continue
        fi

        command="mv ${deviceName}_index.txt archive_index.txt"
        if ! $command ; then
            echo "    ERROR: '$command' failed in $(pwd)" >&2
            ((numErrors++))
        fi

        # This file may not exist - only if index files should be created:
        propWithIndexFile=${deviceName}_properties_with_index.txt
        if [ -e $propWithIndexFile ]; then
            command="mv ${propWithIndexFile} properties_with_index.txt"
            if ! $command ; then
                echo "    ERROR: '$command' failed in $(pwd)" >&2
                ((numErrors++))
            fi
        fi

        command="mv ${deviceName}_schema.txt archive_schema.txt"
        if ! $command ; then
            echo "    ERROR: '$command' failed in $(pwd)" >&2
            ((numErrors++))
        fi

        command="mv ${deviceName}.last archive.last"
        if ! $command ; then
            echo "    ERROR: '$command' failed in $(pwd)" >&2
            ((numErrors++))
        fi

        counter=0
        prefixToRemove=${deviceName}_configuration_
        for oldRawFile in ${prefixToRemove}*.txt ; do
            newRawFile=archive_${oldRawFile#$prefixToRemove}
            command="mv $oldRawFile $newRawFile"
            if ! $command ; then
                echo "      ERROR: '$command' failed in $(pwd)." >&2
                ((numErrors++))
            else
                ((counter++))
            fi
        done
        echo "    Converted $counter raw files for device '${deviceName}'."
        # Go back to be able to cd to next device directory.
        cd ../../
    done
    echo "  Converted ${countDevices} device directories in '${historyDir}'."
done

echo
echo "Treated ${numDirectories} directories (out of $# in arguments): ${numErrors} errors."
