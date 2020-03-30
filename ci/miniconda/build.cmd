
REM clean any remaining stuff from previous jobs
git clean -fxd

REM clean conda envs to avoid vs2015_runtime-like link errors on windows
call conda remove -n karabogui --all -y
call conda build purge-all

REM Environment used for bootstrapping
call conda devenv
call conda activate karabogui

PUSHD .\src\pythonGui\
REM generate version file that is used by conda build
call python setup.py --version
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
POPD

PUSHD .\conda-recipes\karabogui\
REM create recipe needed by conda
call python -m cogapp -d -o meta.yaml meta_base.yaml
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
echo "**********Building KaraboGui with the following Recipe**********"
type .\conda-recipe\meta.yaml

REM bundle karabogui in conda
call conda build ./conda-recipe -c conda-forge -c http://exflserv05.desy.de/karabo/channel -c defaults --override-channels --no-anaconda-upload
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

REM generate mirror directory
set MIRROR_PATH=%TEMP%\mirror\
RMDIR /Q/S %MIRROR_PATH%
call python .\scripts\create_mirror_channels.py --target_dir %MIRROR_PATH% --env karabogui
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
POPD