#!/usr/bin/env bash

export KARABO=$CONDA_PREFIX
export KARABO_EXTERN=$CONDA_PREFIX
export OLD_PYTHONPATH=$PYTHONPATH

if [ ! -d "${KARABO}/var/data" ]; then
    mkdir -p "${KARABO}/var/data"
fi

if [ ! -d "${KARABO}/var/log" ]; then
    mkdir -p "${KARABO}/var/log"
fi

if [ ! -d "${KARABO}/plugins" ]; then
    mkdir -p "${KARABO}/plugins"
fi

pushd "$KARABO/var/environment" >/dev/null
for var in $(ls)
do
   export $var="$(cat $var)"
done
popd >/dev/null

if [ ! -d "${KARABO}/extern" ]; then
    ln -s $CONDA_PREFIX $CONDA_PREFIX/extern
fi
