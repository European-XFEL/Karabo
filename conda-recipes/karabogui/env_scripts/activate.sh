#!/bin/bash
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

# Store existing OMP_NUM_THREADS value

if [[ -n "$OMP_NUM_THREADS" ]]; then
    export _CONDA_SET_OMP_NUM_THREADS=$OMP_NUM_THREADS
fi

if [[ -z "$LANG" ]]; then
    export _CONDA_UNSET_LANG=true
    export LANG="en_US.UTF-8"
fi
export OMP_NUM_THREADS=1