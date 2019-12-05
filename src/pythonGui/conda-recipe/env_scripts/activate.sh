#!/bin/bash

# Store existing OMP_NUM_THREADS value

if [[ -n "$OMP_NUM_THREADS" ]]; then
    export _CONDA_SET_OMP_NUM_THREADS=$OMP_NUM_THREADS
fi

export OMP_NUM_THREADS=1