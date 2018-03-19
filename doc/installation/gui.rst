*******************
 Windows GUI client
*******************
In order to install and run the karabo GUI client on Windows, you need a python
installation.

The recommended python distribution is `WinPython
<https://sourceforge.net/projects/winpython/files/WinPython_3.4/3.4.3.3/>`_.
This choice was made due to the fact that this suite is dictated
by availability of almost all packages required by karaboGUI (such as PyQwt,
guiqwt, guidata).


Updating
=========
When updating version, ensure to remove the previous installation before 
continuing, as otherwise the GUI may have unpredictable behavior.
Navigate to::

 [WinPython_Installation_Dir]\python-3.4.3\Lib\site-packages

Then remove the *karabo* and *karabogui* folders, as well as the *KaraboGUI-\*.egg-info*
file.


Installation instructions
=========================
The installation consists of four steps:
- installing WinPython
- upgrading packages
- installing additional dependencies
- installing KaraboGui, creating a shortcut


Installing WinPython
--------------------
Download the WinPython 3.4 **32Bits** installation package
`here <https://sourceforge.net/projects/winpython/files/WinPython_3.4/3.4.3.3/WinPython-32bit-3.4.3.3.exe/download>`_
and install it following
`these instructions <https://github.com/winpython/winpython/wiki/Installation>`_.


Upgrading and installing additional packages
--------------------------------------------
Before installing Karabo, certain packages need to be installed, and a few more
installed.

To do so, open the `WinPython Command Prompt` and type in the following 
commands.

First, upgrade `pip`, `setuptools`, and `matplotlib`::

  pip install --upgrade pip
  pip install --upgrade setuptools
  pip install --no-deps matplotlib==1.5.3


There are four additional packages needed:
- `suds-jurko <http://pypi.python.org/packages/source/s/suds-jurko/suds-jurko-0.6.zip>`_
- `traits 4.6.0 <https://pypi.python.org/pypi/traits>`_
- `pint 0.7.2 <https://pypi.python.org/pypi/Pint/>`_
- `cycler 0.10.0 <https://pypi.python.org/pypi/cycler/>`_

Install these a follows::

    pip install suds-jurko==0.6
    pip install traits==4.6.0
    pip install pint==0.7.2
    pip install cycler==0.10.0


Get and install karaboGUI
-------------------------
Download `karabo-gui for windows from here <http://exflserv05.desy.de/karabo/karaboGui/>`_ .
Then, open the `WinPython Control Panel`, found in the WinPython installation 
folder and load in the downloaded executable.
Finally, click `Install packages`.

Desktop Shortcut
----------------
Currently there is no Start Menu entry or a shortcut on the Desktop. This needs
to be created by going to the path where WinPython is installed::

 [WinPython_Installation_Dir]\python-3.4.3\Lib\site-packages\karabogui\programs

Right-click on *gui_runner.py* then select *Send to Desktop*. 
You can now easily start karaboGui from the desktop. 

If Windows prompts you to ask what software to use to launch this file, select::

 [WinPython_Installation_Dir]\python-3.4.3\python.exe


Uninstalling
============
Navigate to::

 [WinPython_Installation_Dir]\python-3.4.3\Lib\site-packages

Then remove the *karabo* and *karabogui* folders, as well as the *KaraboGUI-\*.egg-info*
file.

WinPython is a portable application. To uninstall it, delete its folder.
