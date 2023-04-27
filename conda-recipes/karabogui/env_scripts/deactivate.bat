:: Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
@REM Restore previous OMP_NUM_THREADS value

@set "OMP_NUM_THREADS="
@if defined _CONDA_SET_OMP_NUM_THREADS (
    set "OMP_NUM_THREADS=%_CONDA_SET_OMP_NUM_THREADS%"
    set "_CONDA_SET_OMP_NUM_THREADS="
)
