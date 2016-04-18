#!/bin/bash

BROKER=exfl-broker:7777

if [ ${#@} -le 0 ] || [ $1 == "-h" ] || [ $1 == "--help" ]; then
    # print help message if first argument asks for help
    echo
    echo "$0 - Look into messages stuck on broker:"
    echo 
    echo "  $0 [-h|--help] topic [broker:port [firstMessageIdx [numMessages]]]"
    echo
    echo "Default broker:port is '$BROKER'."
    echo "Must be called from a directory where './queryBroker' is available."
    echo
    exit 0
fi

TOPIC=$1

if [ ${#@} -ge 2 ]; then
    BROKER=$2
fi

# Command gives something like:
# ...
#-----------------------------------------------------------------------------------
#Message #   Message IDs                                     Priority   Body Type
#-----------------------------------------------------------------------------------
#0           ID:15-192.168.140.159-59611-1459519722221       4          BytesMessage
#1           ID:8-192.168.140.159-59620-1459519722615        4          BytesMessage
# ...
COMMAND="./queryBroker -b $BROKER list msg -t t -n $TOPIC -nocheck"

if [ ${#@} -ge 3 ]; then
    COMMAND="$COMMAND -startMsgIndex $3"
    if [ ${#@} -ge 4 ]; then
        COMMAND="$COMMAND -maxMsgsRet $4"
    fi
fi

declare -i counter=0

for msgID in `$COMMAND | grep 'ID:' | awk '{print $2}' `; do
    echo "message query no. ${counter}: " $ID
    ./queryBroker -b $BROKER query msg -t t -n $TOPIC -nocheck -msgID $msgID
    echo
    echo "=================================================================="
    echo
    counter+=1
done
