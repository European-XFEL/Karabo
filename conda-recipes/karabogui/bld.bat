:: Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
set BUILD_KARABO_SUBMODULE=NATIVE
cd ./src/pythonKarabo
python setup.py install

cd ../pythonGui
python setup.py install

:: from https://docs.conda.io/projects/conda-build/en/latest/resources/activate-scripts.html
setlocal EnableDelayedExpansion

:: Copy the [de]activate scripts to %PREFIX%\etc\conda\[de]activate.d.
:: This will allow them to be run on environment activation.
for %%F in (activate deactivate) DO (
    if not exist %PREFIX%\etc\conda\%%F.d mkdir %PREFIX%\etc\conda\%%F.d
    copy %RECIPE_DIR%\env_scripts\%%F.bat %PREFIX%\etc\conda\%%F.d\%PKG_NAME%_%%F.bat
    :: Copy unix shell activation scripts, needed by Windows Bash users
    copy %RECIPE_DIR%\env_scripts\%%F.sh %PREFIX%\etc\conda\%%F.d\%PKG_NAME%_%%F.sh
)
