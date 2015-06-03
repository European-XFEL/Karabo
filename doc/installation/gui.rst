***********************
 Only GUI for Windows 7
***********************

*To be done by M. Teichmann and J. Szuba*

In order to install and run karaboGUI on Windows, you need a local python installation. 
The recommended python suites are `Python(x,y) <https://code.google.com/p/pythonxy/>`_ or `WinPython <http://winpython.github.io/>`_. 
However, as python(x,y) still lacks version with Python 3, currently the only possibility is to use WinPython. 
The choice of python suite is dictated by availability of almost all packages required by karaboGUI (like PyQwt, guiqwt, guidata).

Additionally, you need to install one module called suds-jurko in Python(x,y) or WinPython using their package managers.

Below you will find detailed instruction how to install WinPython and karaboGUI.

In principle you can try to use any other available Scientific Python distributions like:

- `Anaconda <http://continuum.io/downloads>`_: A free distribution for the SciPy stack. Supports Linux, Windows and Mac
- `Enthought Canopy <http://www.enthought.com/products/canopy/>`_: The free and commercial versions include the core SciPy stack packages. Supports Linux, Windows and Mac, but only Python 2.7
- `Pyzo <http://www.pyzo.org/>`_: A free distribution based on Anaconda and the IEP interactive development environment. Supports Linux, Windows and Mac

and then, using appropiate package manager, or simply pip, install additional packages.
You could use pure `Python <https://www.python.org/downloads/>`_, but then you need to take care and install all required additional Python packages.

For Windows, Christoph Gohlke provides `pre-built Windows installers <http://www.lfd.uci.edu/~gohlke/pythonlibs/>`_ for many Python packages in wheels format (see `Installing Packages <https://packaging.python.org/en/latest/installing.html>`_ for explanation of wheel format).

Here is a list of all python packages included in karaboFramework, not all are required for karaboGUI, some of them are inter-dependent:

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

In case your installed python distribution includes slightly different version of these packages, pythonGUI should nevertheless work.

Installation instructions
=========================

Get and install WinPython
-------------------------

Download the latest installation package `here <https://sourceforge.net/projects/winpython/files/WinPython_3.4/3.4.3.3/>`_ (currently 3.4.3.3 using Python 3.4.3) and install it following `instructions <https://github.com/winpython/winpython/wiki/Installation>`_. After installation register your WinPython distribution to Windows.

Get and install additional python packages
------------------------------------------

To download the additional package suds-jurko 0.6 follow these instructions:

- from the WinPython installation directory, launch the WinPython Command Prompt.
- then use pip at the prompt to install the package typing 'pip install suds-jurko'.



Get and install karaboGUI
-------------------------

Download karaboGUI Windows installation binary `here <ftp://karabo:framework@ftp.desy.de/karaboGui/>`_ and run it. NOTE: if this link does not open automatically, type the following
in the browser address bar: ftp://karabo:framework@ftp.desy.de/karaboGui/ .
 You should get Start Menu entry as well as karaboGUI shortcut on Desktop.

To uninstall karaboGui, open Control Panel -> Uninstall a program, find karaboGUI entry and uninstall it.


**IMPORTANT**

Due to `this issue <http://bugs.python.org/issue21354>`_ (resolved in Python 3.4.4rc1, not yet released), the installer gives an error at the end ('python not found'), the menu entry and shortcut are not created. To start karaboGUI you need to navigate to::

 [WinPython_Installation_Dir]\python-3.4.3.amd64\Lib\site-packages\karaboGui 

right-click on karabo-gui.py and  select Send to Desktop. In this way, you have a shortcut on your Desktop and now you can easily start karaboGui with double click. To remove karaboGui, you need to use WinPython package manager: select karabo and karaboGui and press Uninstall packages.


