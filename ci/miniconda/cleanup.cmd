PUSHD %WIN_CONDA_ROOT%

REM conda cannot remove directories on windows.
REM the origin of the issue seems to be the fact that the main
REM dll files are linked from the base environment.
REM For some reason, windows will lock the files in the environment
REM to the executable of the CI (python) preventing the CI
REM from cleaning itself. Here we choose the nuclear option of
REM wiping everything clean.

REM clean the conda-bld directory
IF EXIST conda-bld (
    PUSHD conda-bld
    DEL /q /f /s * > nul
    POPD
    RMDIR /q /s conda-bld
)
MKDIR conda-bld

REM remove all environments
IF EXIST envs (
    PUSHD envs
    DEL /q /f /s * > nul
    POPD
    RMDIR /q /s envs
)
MKDIR envs
POPD
