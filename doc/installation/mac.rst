**********************************************************
Only GUI for MacOS (currently unmaintained, use with care)
**********************************************************

In order to install and run karaboGUI on MacOS, you need a local python installation.
None of the known python scientific distribution (anaconda, canopy express, etc) contains pyqwt, guidata, guiqwt, so using macport is recommended.

Get and install python and required packages
============================================

1. Install Xcode, then install the Command Line Tools of Xcode. For that open Xcode, and navigate to Xcode->Preferences->Downloads->Components and click "Install".
   You can also install only Command Line Tools without installing full Xcode, just type in terminal 'gcc', then in pop-up window click 'Install' button.
2. Put in .profile proper locale (otherwise you will get error from guidata, or karabo gui)::

    export LC_ALL=en_US.UTF-8
    export LANG=en_US.UTF-8

3. Download and install XQuartz (.dmg) from this location: http://xquartz.macosforge.org/landing/
4. Customize xterm so that it respects .profile::

    Open X11 and select Customize... from the Applications menu, double-click the menu item Terminal and change: “xterm” to “xterm -ls” (this means login shell)

5. Install MacPorts (install .pkg from www.macports.org)
6. Install relevant packages using MacPorts. Go to a terminal and type::

    sudo port install gcc49
    sudo port select --set gcc mp-gcc49
    sudo port install -f dbus
    sudo port install sqlite3 qt4-mac-sqlite3-plugin python34
    sudo port select --set python python34
    sudo port install py34-pyqt4
    sudo port install py34-Pillow
    sudo port install py34-numpy py34-scipy
    sudo port install py34-matplotlib +qt4
    sudo port install py34-pyqwt  (not existing yet, see manual installation below)
    sudo port install py34-cython
    sudo port select --set cython cython34
    sudo port install py34-ipython +notebook +pyqt4
    sudo port select --set sphinx py34-sphinx
    sudo port select --set ipython ipython34
    sudo port select --set nosetests nosetests34
    sudo port install py34-suds-jurko
    sudo port install py34-setuptools py34-pip
    sudo port select --set pip pip34
    sudo port install doxygen

8. Install guidata and guiqwt::

    cd ~/Downloads
    wget http://guidata.googlecode.com/files/guidata-1.6.1.zip
    unzip guidata-1.6.1.zip
    cd guidata-1.6.1
    python setup.py install --user

    cd ~/Downloads
    wget http://guiqwt.googlecode.com/files/guiqwt-2.3.1.zip
    unzip guiqwt-2.3.1.zip
    cd guiqwt-2.3.1
    python setup.py install --user

9. Download PyQwt `here <http://prdownloads.sourceforge.net/pyqwt/PyQwt-5.2.0.tar.gz?download>`_, download patchPyQwt `here <ftp://karabo:framework@ftp.desy.de/karaboGui/>`_. Untar PyQwt, patch it and install::

    patch -p0 < patchPyQwt


    cd PyQwt-5.2.0/configure
    python -c "from lib2to3.main import main;main('lib2to3.fixes')" -w configure.py
    python configure.py -Q ../qwt-5.2
    make
    sudo make install





Get and install karaboGui source distribution
=============================================

Download karaboGUI source distribution package `here <ftp://karabo:framework@ftp.desy.de/karaboGui/>`_, untar it and change directory to karaboGui.
Run::

  python setup.py install

or::

  python setup.py install --user

if you want to put all modules included in KaraboGui in user space.

After succesfull installation, run karaboGui python script in scripts folder.

During installation you can add option "--record files.txt", like::

  python setup.py install --user --record files.txt

then in the file files.txt you can find full list of installed files.
Afterwards, you can uninstall KaraboGui with::

  cat files.txt | xargs rm -rf


