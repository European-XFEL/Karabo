******************
Only GUI for MacOS
******************

In order to install and run karaboGUI on MacOS, you need a local python installation.
None of the known python scientific distribution (anaconda, canopy express, etc) contains pyqwt, guidata, guiqwt, so using macport is recommended.

Get and install python and required packages
============================================

1. Install Xcode
2. Install the Command Line Tools of Xcode. For that open Xcode, and navigate to Xcode->Preferences->Downloads->Components and click "Install".
3. Put in .profile proper locale (otherwise you will get error from guidata, or karabo gui)::

    export LC_ALL=en_US.UTF-8
    export LANG=en_US.UTF-8

4. Download and install XQuartz (.dmg) from this location: http://xquartz.macosforge.org/landing/
5. Customize xterm so that it respects .profile::

   Open X11 and select Customize... from the Applications menu, double-click the menu item Terminal and change: “xterm” to “xterm -ls” (this means login shell)

6. Install MacPorts (install .dmg from www.macports.org)
7. Install relevant packages using MacPorts. Go to a terminal and type::

    sudo port install gcc48
    sudo port select --set gcc mp-gcc48
    sudo port install sqlite3 qt4-mac-sqlite3-plugin python34
    sudo port select --set python python34
    sudo port install py34-pyqt4
    sudo port install py34-numpy py34-scipy
    sudo port install py34-matplotlib +qt4
    sudo port install py34-pyqwt
    sudo port install py34-cython
    sudo port select --set cython cython34
    sudo port install py34-ipython -scientific +notebook +pyqt4
    sudo port select --set sphinx py34-sphinx
    sudo port select --set ipython ipython34
    sudo port select --set nosetests nosetests34
    sudo port install py34-suds-jurko

8. Install quamash, guidata and guiqwt::

    cd ~/Downloads
    wget https://pypi.python.org/packages/source/Q/Quamash/Quamash-0.4.1.tar.gz
    tar xvzf Quamash-0.4.1.tar.gz
    cd Quamash-0.4.1
    python setup.py install --user

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


