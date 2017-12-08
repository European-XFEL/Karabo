***********************
 Only GUI for Windows 7
***********************

In order to install and run karaboGUI on Windows, you need a local python
installation.

The recommended python suite is `WinPython
<https://sourceforge.net/projects/winpython/files/WinPython_3.4/3.4.3.3/>`_.
This choice was made due to the fact that this suite is dictated
by availability of almost all packages required by karaboGUI (like PyQwt,
guiqwt, guidata).

Additionally, you need to install three more modules (suds-jurko, traits, pint)
in WinPython using its package manager (for instance WinPython Control
Panel).

Below you will find detailed instruction :ref:`how to install WinPython and
karaboGUI <install-winpython>`.

In principle you can try to use any other available Scientific Python
distributions like:

- `Python(x,y) <https://code.google.com/p/pythonxy/>`_: A free distribution for
   Windows but does not yet support Python 3
- `Anaconda <http://continuum.io/downloads>`_: A free distribution for the SciPy
   stack. Supports Linux, Windows and Mac
- `Enthought Canopy <http://www.enthought.com/products/canopy/>`_: The free and
   commercial versions include the core SciPy stack packages. Supports Linux,
   Windows and Mac, but only Python 2.7
- `Pyzo <http://www.pyzo.org/>`_: A free distribution based on Anaconda and the
   IEP interactive development environment. Supports Linux, Windows and Mac

and then, using an appropiate package manager, or simply pip, install additional
packages.
You could use pure `Python <https://www.python.org/downloads/>`_, but then you
need to take care and install all required additional Python packages.

For Windows, Christoph Gohlke provides
`pre-built Windows installers <http://www.lfd.uci.edu/~gohlke/pythonlibs/>`_
for many Python packages in wheels format
(see `Installing Packages <https://packaging.python.org/en/latest/installing.html>`_
for explanation of wheel format).

Here is a list of all python packages included in karaboFramework, not all are
required for karaboGUI, some of them are inter-dependent:

- backports.ssl_match_hostname 3.4.0.2
- Cython 0.20
- docutils 0.12
- python-dateutil 2.2
- ecdsa 0.11
- guidata 1.6.1
- guiqwt 2.3.1
- h5py 2.2.1
- httplib2 0.9.1
- ipython 3.0.0
- jsonschema 2.3.0
- Jinja2 2.7.2
- MarkupSafe 0.18
- matplotlib 1.3.1
- nose 1.3.0
- numpy 1.8.0
- paramiko 1.14.0
- parse 1.6.3
- pexpect 3.1
- Pillow 2.5.3
- pssh 2.3.1
- pycrypto 2.6.1
- Pygments 1.6
- pyparsing 2.0.1
- PyQt-x11-gpl 4.11.3
- PyQwt 5.2.0
- Python 3.4.3
- pytz 2013.9
- pyusb 1.0.0b1
- pyzmq 14.0.1
- scipy 0.13.2
- setuptools 2.1
- sip 4.16.7
- six 1.5.2
- Sphinx 1.2.3
- suds-jurko 0.6
- tornado 3.2
- tzlocal 1.1.1

In case your installed python distribution includes slightly different version
of these packages, pythonGUI should nevertheless work.

.. warning:
Since Karabo 2.1.18.2, `matplotlib 1.5.3 <https://pypi.python.org/packages/5e/06/6a717e37f0bb331bf152bfeb7ded4060f1188b508631e988c1cdbef5a8ab/matplotlib-1.5.3-cp34-cp34m-win32.whl#md5=c0705c7d2278f557eac4c1c2e75245d5>`_ and `cycler 0.10.0 <https://pypi.python.org/packages/f7/d2/e07d3ebb2bd7af696440ce7e754c59dd546ffe1bbe732c8ab68b9c834e61/cycler-0.10.0-py2.py3-none-any.whl#md5=2820ec00c7dd68487bde1a7cdb165904>`_ are required by the GUI

.. _install-winpython:

Installation instructions
=========================

Get and install WinPython
-------------------------

Download the latest installation package
`here <https://sourceforge.net/projects/winpython/files/WinPython_3.4/3.4.3.3/WinPython-32bit-3.4.3.3.exe/download>`_
(**currently 3.4.3.3 using Python 3.4.3**) and install it following
`instructions <https://github.com/winpython/winpython/wiki/Installation>`_.
After installation register your WinPython distribution to Windows. From now on,
you have Start Menu entry for WinPython.

.. _install-addons:

Get and install additional python packages
------------------------------------------

There are three additional packages needed:

- `suds-jurko <http://pypi.python.org/packages/source/s/suds-jurko/suds-jurko-0.6.zip>`_

- `traits 4.6.0 <https://pypi.python.org/pypi/traits>`_

- `pint 0.7.2 <https://pypi.python.org/pypi/Pint/>`_


To intall the additional packages follow these instructions:

- from the WinPython installation directory, launch the WinPython Command Prompt
- then use pip at the prompt to install the package typing 'pip install <packagename>'.

or use WinPython Control Panel:

- download the package
- open WinPython Control Panel, drag&drop downloaded file and install it
  (WinPython instructions
  `here <https://github.com/winpython/winpython/wiki/Installing-Additional-Packages>`_).


Get and install karaboGUI
-------------------------

Download karaboGUI Windows installation binary
`here <http://exflserv05.desy.de/karabo/karaboGui/>`_ and install it as it was described
for the :ref:`additional packages <install-addons>`.

Currently there is no Start Menu entry or a shortcut on the Desktop. This needs
to be created by going to the path where WinPython is installed::

 [WinPython_Installation_Dir]\python-3.4.3\Lib\site-packages\karabo_gui

and then right click on *main.py* and select *Send to Destop (create shortcut)*

To uninstall karaboGui, open Control Panel -> Uninstall a program, find
karaboGUI entry and uninstall it.


**IMPORTANT**

Due to `this issue <http://bugs.python.org/issue21354>`_
(resolved in Python 3.4.4rc1, not yet released), the installer gives an error at
the end ('python not found'), the menu entry and shortcut are not created. To
start karaboGUI you need to navigate to::

 [WinPython_Installation_Dir]\python-3.4.3\Lib\site-packages\karabo_gui\programs

Right-click on *gui_runner.py* and  select *Send to Desktop*. In this way, you have
a shortcut on your *Desktop* and now you can easily start karaboGui via
mouse-double-click. To remove karaboGui, you need to use WinPython package
manager: Select karabo and karaboGui and press Uninstall packages.

**UPGRADE KARABO**

Make sure to remove the old karabo version before install new version of karabo.
Otherwise you will encounter unpredicted behavior of the GUI, before installing
the new version, you need to navigate to::

 [WinPython_Installation_Dir]\python-3.4.3\Lib\site-packages

Remove folder *karabo* and *karabo_gui*, also remove the file *KaraboGUI-\*.egg-info*

**UPGRADING PACKAGES**

If you desire to use newer packages of certain libraries, such as matplotlib and
cycler in a newer installation of Karabo you will need to upgrade `pip` and
`setuptools` first::

  pip install --upgrade pip
  pip install --upgrade setuptools

  # Then proceed to matplotlib and cycler updates
  pip install --no-deps matplotlib==1.5.3
  pip install --no-deps cycler==0.10.0
