:: Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
@REM Store existing OMP_NUM_THREADS

@if defined OMP_NUM_THREADS (
    set "_CONDA_SET_OMP_NUM_THREADS=%OMP_NUM_THREADS%"
)
@set "OMP_NUM_THREADS=1"
