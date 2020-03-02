REM HOME variable is current set on our gitlab-ci.yml and is needed so our
REM ssh work correctly
set SSH_KEY=%HOME%\.ssh\win-cwrsync
set REMOTE_SERVER=xkarabo@exflserv05
set CONDA_CHANNEL_PATH=/var/www/html/karabo/channel/
set REMOTE_CHANNEL_PATH=%REMOTE_SERVER%:%CONDA_CHANNEL_PATH%
set PLATFORM=win-64

REM get conda-bld directory
set CONDA_OUT=%TEMP%\conda_info_out.txt
call conda info --root > %CONDA_OUT%
set /P PKG_FOLDER=<%CONDA_OUT%

REM As we are using cwrsync on Windows, we can't use any paths containing :
REM as it will interpret it as a remote server. Therefore, we use the path
REM /cygdrive/c/Users/xkarabo/ etc, which cwrsync expected.

REM find the gui package
set KARABOGUI_PKG=%PKG_FOLDER%\conda-bld\win-64\karabogui-%CI_COMMIT_REF_NAME%*.tar.bz2
REM remove :
set KARABOGUI_PKG=%KARABOGUI_PKG::=%
REM replace \ to / and add prefix
set KARABOGUI_PKG=/cygdrive/%KARABOGUI_PKG:\=/%

REM upload conda package to remote
rsync --rsh="ssh -i %SSH_KEY%" ^
      --progress %KARABOGUI_PKG% %REMOTE_CHANNEL_PATH%/%PLATFORM%/ ^
      --chmod=644
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

REM format mirror path to cwrsync standard
set MIRROR_PATH=%TEMP%\mirror\
set MIRROR_PATH=%MIRROR_PATH::=%
set MIRROR_PATH=/cygdrive/%MIRROR_PATH:\=/%

REM upload mirror repository
call rsync -r --exclude ".git" ^
              --rsh="ssh -i %SSH_KEY%" ^
              --progress %MIRROR_PATH%/* %REMOTE_CHANNEL_PATH%/mirror/ ^
              --chmod=644
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

REM rebuild remote channels
echo y | plink -no-antispoof -pw %XKARABO_PWD% %REMOTE_SERVER% bash "source ~/miniconda3/bin/activate; cd %CONDA_CHANNEL_PATH%; conda index .;"
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%