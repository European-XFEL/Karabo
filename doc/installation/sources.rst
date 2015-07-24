.. _buildFromSources:

*******************
 Build from sources
*******************

Install dependencies
====================

All
---

In new openmq 5.0 admin password is encoded using bundled tool - you
also need to install java before compiling karaboFramework to get
properly configured local broker - see Netbeans instructions :ref:`how to
install java <installingJava>`.


Ubuntu type system
------------------

.. code-block:: bash

  sudo apt-get install subversion build-essential doxygen libqt4-dev libnss3-dev libnspr4-dev libreadline-dev libsqlite3-dev libqt4-sql-sqlite libX11-dev zlib1g-dev gfortran liblapack-dev m4 libssl-dev

Additionally for Ubuntu 10.04:

.. code-block:: bash

  sudo apt-get install libxext-dev

.. code-block:: bash

  sudo apt-get install krb5-user

For krb5-user use 'DESY.DE' (without quotes) as default REALM if you
are within DESY network.

For running karabo python tests:

.. code-block:: bash

  sudo apt-get install imagemagick


SL6
---

.. code-block:: bash

  sudo yum install redhat-lsb make gcc gcc-c++ gcc-gfortran subversion doxygen nspr-devel nss-devel zlib-devel libX11-devel readline-devel qt-devel lapack-devel sqlite-devel openssl-devel
  sudo yum install epel-release
  sudo yum --enablerepo=epel install qtwebkit-devel
  sudo yum install krb5-workstation

For krb5-workstation copy /etc/krb5.conf from any DESY server (eg. from exflserv02) (i.e. copy /etc/krb5.conf from the DESY server to the /etc/ directory of your local machine, overwriting your own /etc/krb5.conf file).

For running karabo python tests:

.. code-block:: bash

  sudo yum install imagemagick


Fedora
------

.. code-block:: bash

  sudo yum install make gcc gcc-c++ gcc-gfortran subversion doxygen  nspr-devel nss-devel zlib-devel libX11-devel readline-devel qt-devel  lapack-devel sqlite-devel openssl-devel
  sudo yum install krb5-workstation

For krb5-workstation use DESY.DE as default REALM.

For running karabo python tests:

.. code-block:: bash

  sudo yum install imagemagick


MacOS X
-------

1. Install Xcode

2. Install the Command Line Tools of Xcode. For that open Xcode, and navigate to Xcode->Preferences->Downloads->Components and click "Install".

3. Install MacPorts (install .dmg from http://www.macports.org)

4. Go to a terminal and type:

  .. code-block:: bash

    sudo port install gcc48
    sudo port select --set gcc mp-gcc48
    sudo port install nspr nss pkgconfig sqlite3 qt4-mac-sqlite3-plugin python34
    sudo port select --set python python34
    sudo port install py34-pyqt4
    sudo port install py34-numpy py34-scipy
    sudo port install py34-matplotlib +qt4
    sudo port install py34-cython
    sudo port select --set cython cython34
    sudo port install py34-ipython -scientific +notebook +pyqt4
    sudo port select --set sphinx py34-sphinx
    sudo port select --set ipython ipython34
    sudo port select --set nosetests nosetests34
    sudo port install py34-suds-jurko gdb
    sudo port install ImageMagick

  Comments:

  Starting from Xcode 5 there is no gcc included (only clang), so gcc
  4.8 from macports is installed. For some packages variants are
  enabled/disabled (for matplotlib 'qt4' instead of 'tk' frontend, for
  ipython 'scientific' is disabled not to pull hdf5 from
  macports). For all 'package'_select the default binary is set.

  There is no gdb in Xcode CLI DEvelopers Tools, you can install it
  from Macports, notice that name if the executable is ggdb.

  As we are using gcc 4.8 from macport for karabo framework
  compilation, you need to add a new toolchain in Netbeans (with the
  name GNU_MacPorts).

  Installation of guidata, guiqwt, h5py and parse is done similarily
  as in Linux. However they are installed in user space
  (~/Library/Python/2.7) so that they don't interfere with other
  python packages installed through Macports and this location is
  automaticaly added to python search path. h5py is available in
  macports but requires hdf5-18 from macports - then it may conflicts
  with hdf5 shipped with karabo extern. pyqwt5 is installed in the
  System Python site-packages folder, that't the reason it requires
  the password to sudo command.

5. Add a new toolchainin Netbeans: Open Preferences->C/C++->Build
   Tools. Add new Tool Collection - press Add... Fill in Base Directory
   to : /opt/local/bin. Give a ToolCollection Name "GNU_MacPorts". Make
   it default.

6. Patch NetBeans bug regarding Makefile paths

  .. code-block:: bash

    cd /usr/bin
    sudo ln -sf /opt/local/bin/pkg-config pkg-config

7. Create a symbolic link to python includes (boost needs this):

  .. code-block:: bash

    cd /opt/local/Library/Frameworks/Python.framework/Versions/Current/include
    sudo ln -sf python3.4m python3.4

8. Put in .profile proper locale (otherwise you will get error from
   guidata, or karabo gui)

  .. code-block:: bash

    export LC_ALL=en_US.UTF-8
    export LANG=en_US.UTF-8

9. Download and install XQuartz (.dmg) from this location:
   http://xquartz.macosforge.org/landing/


  Customize xterm so that it respects .profile:

  Open X11 and select Customize... from the Applications menu,
  double-click the menu item Terminal and change: “xterm” to “xterm
  -ls” (this means login shell)

10. Put the following line to your .profile file:

  .. code-block:: bash

    export DYLD_LIBRARY_PATH=$(cat ~/.karabo/karaboFramework)/extern/lib:$(cat ~/.karabo/karaboFramework)/lib
    export PYTHONPATH=$(cat ~/.karabo/karaboFramework)/extern/lib:$(cat ~/.karabo/karaboFramework)/lib
 
11. There may be mismatch between subversion command line client
    version and svn client included in Netbeans (Netbeans 8 svnkit
    client support 1.6 and 1.8). On Mavericks svn client included in
    Xcode is 1.7. If the project was checked out using command line
    client then Netbeans will upgrade (if you say yes) local working
    directory of the project to its svn version - then you cannot work
    with command line client any more. Also the other way around. You
    may bring back command line functionality by installing subversion
    from MacPorts, but this require changing default build option for
    serf1 library responsible for connecting to svn repository using
    http/https so that it also includes gssapi/kerberos authentication
    features. If you don't want to play with recompilation, then Xcode
    5 also ships subverions version 1.6 in the following directory:
    /Applications/Xcode.app/Contents/Developer/usr/subversion-1.6/bin/svn. You
    would need to use full path or create an alias or symbolic
    link. In the end, you can also decide if you use only Netbeans svn
    client or only command line client to avoid any problems. For
    details see also :ref:`netbeans`.


12. Hint for karabo Framework: If you checkout fresh copy, then run
    ./auto_build_all.sh Debug/Release. If you had already local
    working copy, go to build/netbeans/karabo and clean extern with:
    make clean-extern. Then go back to karabo top folder and run
    ./auto_build_all.sh Clean, followed by ./auto_build_all.sh
    Debug/Release. You can also compile in Netbeans, then makefiles
    are updated automatically.

  Historical remark: how to install older version of package from Macports

  .. code-block:: bash

    # Create a folder for a local repository of ipython macport
    mkdir /Users/Shared/dports

    # add this repository so that port command will see it
    # edit the following file
    sudo vim /opt/local/etc/macports/sources.conf
    # and put this before rsync line: file:///Users/Shared/dports

    # now checkout a proper revision (you have to find in trunk the revision number relevant for you, or google for it)
    # in this example this was the last revision for ipython 0.13.2
    cd /Users/Shared/dports
    svn co -r 108534 http://svn.macports.org/repository/macports/trunk/dports/python/py-ipython python/py-ipython

    # run portindex
    portindex /Users/Shared/dports

    # you can check beforehand that you can see old port
    port list py34-ipython


.. _netbeans:

NetBeans
========

NetBeans is the recommended IDE for working on the Karabo
framework. See :ref:`here <gettingStartedNetbeans>` how to install and
set up correctly.


Get and install the Karabo framework (quick - no details)
=========================================================

1. Create the folder where you want to check out the karaboFramework
   and go there, e.g.

  .. code-block:: bash

    mkdir karaboFramework
    cd karaboFramework

2. Before the check out, get a valid kerberos ticket via:

  .. code-block:: bash

    kinit username@DESY.DE

3. Now check out:

  .. code-block:: bash

    svn co https://svnsrv.desy.de/desy/EuXFEL/WP76/karabo/karaboFramework/trunk .

4. Compile the Karabo bundle by running the following command for the
   Debug version

  .. code-block:: bash

    ./auto_build_all.sh Debug

  Or this following one for the Release version, respectively.

  .. code-block:: bash

    ./auto_build_all.sh Release

5. (Updated) If you failed building the external dependencies, try and
   fix the error in the dependency and compile again. For reference,
   this file:

  .. code-block:: bash

    karaboFramework/build/netbeans/karabo/.marker.txt

  contains a list of packages installed successfully on your
  machine. Check if you installed all dependencies for your OS listed
  above or try compilation again with:

  .. code-block:: bash

    ./auto_build_all.sh Debug --auto

  but be prepared to enter a sudo password from time to time.


Tips for re-compilation of karaboFramework if you have already your
local working copy.

1. If there were any extern updates (for instance a new package, or a
   new version of existing package)

  * the very clean way is to rebuild all extern with:

    .. code-block:: bash

      cd build/netbeans/karabo
      make clean-extern 
      make extern # or cd ../../../; ./auto_build_all.sh Debug

  * if you know what you are doing:

    .. code-block:: bash

      make extern WHAT="packageA packageB"

2. If there were any changes to netbeans project files like
   configuration.xml or makefiles like Karabo-???.mk

  * clean first and then rebuild:

    .. code-block:: bash

      ./auto_build_all.sh Clean
      ./auto_build_all.sh Debug

  or recompile in NetBeans

3.  If there were only code changes then simply rebuild:

  .. code-block:: bash

    ./auto_build_all.sh Debug

  or recompile in Netbeans
 

Executing Unitary Tests
=======================

Karabo (C++)
------------

To test Karabo Unitary tests please guarantee your local changes are
compiled (via Terminal or Netbeans).
 
To run the Unitary Tests using the Terminal, please go to the
installed karaboFramework folder and execute the following scripts:

.. code-block:: bash

  cd build/netbeans/karabo
  make test
 
To run the Unitary Tests using Netbeans:

* Go to Karabo project
* Right-click on the "Test Files" folder or any of its logic sub-folders
* Select "Test"


PythonKarabo (Python)
---------------------

To test Python code be aware that if you depend on Karathon (and
Karabo C++ code) you must install and deploy the changes you may have
done in Karabo/Karathon in your system.

In Terminal you can do that running:

.. code-block:: bash

  ./auto_build_all.sh Debug/Release

In Netbeans you can do that:

* Right-click in Karabo project Makefile
* Go to "Make Target"
* Select "bundle-install" (if this option doesn't exist, please add it
  using the add button)
 
To run the Unitary Tests using the Terminal, please go to the
installed karaboFramework folder and execute the following scripts:

.. code-block:: bash

  cd /src/pythonKarabo/tests
  nosetests-3.4 -sv tests_api_1
  nosetests-3.4 -sv tests_api_2
  nosetests-3.4 -sv tests_api_2_2
 
To run the Unitary Tests using Netbeans:

1. Go to Tools > Python Platforms
2. Make as Default Python the Python under your current KaraboFramework
   installation

  1. Select "New"
  2. Add python available on your current installation extern folder

    (i.e. /.../your_current_karaboFramework/package/Debug/Ubuntu/14.04/x86_64/karabo/extern/bin/python)
  3. Choose new Python and make it default, selecting "Make Default"
  4. Select Close

3. Run Python Karabo project


Get and install the Karabo framework (all the details)
======================================================

After having checked out the karaboFramework you will find the
following structure of files and sources:

**src/**

  In this directory you will find all Karabo sources. They are
  cleanly split from any build instructions.

  The next hierarchy level reflects the individual projects which are
  part of the KaraboFramework.

  **brokerMessageLogger/**

    Contains sources in C++ and reflects an application that allows
    investigating all messages that are crossing the broker.

  **deviceServer/**

    Contains C++ sources, and builds the generic DeviceServer
    application, which can load Device plugins into the distributed
    system.

  **karabo/**

    The central project, Karabo's backbone in C++. Its directory
    structure is reflected into the namespaces and include hierarchy.

  **karathon/**

    C++ binding layer to make karabo available to the Python
    programming language.

  **karathonTest/**

    The name says it all.

  **pythonCli/**

    Native python code depending on karathon which implements the
    command line interface for Karabo.

  **pythonGui/**

    Native python code using PyQt4 and karathon to implement the
    graphical user interface of Karabo.

  **pythonKarabo/**

    Native python code for some core elements of Karabo which deserve
    a native python re-write instead of a C++ binding.

**build/**

  Contains all build instructions and tools to generate
  libraries/executables and software bundles.

  The three targeted architectures (Linux, MacOS and Windows) are
  separated into two radically different build systems.

  **<projects>/**

    Each directory reflects a regular NetBeans project and can be
    operated directly via NetBeans. The projects reflect those
    mentioned in the src/ directory (see above) one-to-one. NetBeans
    build system was extended to support also builds from
    commandline. Simply type:

    .. code-block:: bash
    
      make CONF=Debug

    or

    .. code-block:: bash

      make CONF=Release 

    for debug or release configuration, respectively.

    HINT: Append the "-j" option to either build command for high-speed parallel build.

    The (central) karabo makefile supports some extra targets to
    trigger creation of a software bundle, which is the way we
    distribute Karabo. A self-extracting install-script for Karabo can
    for example be created by:

    .. code-block:: bash

      make CONF=Debug bundle-package

    or 

    .. code-block:: bash

      make CONF=Release bundle-package

    If you are going to work at the same time on the Karabo framework
    and some packages (plugins) for Karabo you should finalize your
    framework codings with a:

    .. code-block:: bash

      make bundle-install

    Which creates a ready to use bundle under 

    .. code-block:: bash
      package/<Configuration>/<OS>/<Version>/<Arch>/karabo
                  
    and also updates the $HOME/.karabo/karaboFramework file pointing
    to this "local" bundle.

**visualStudio/**

  The inherent (makefile-based) build system of the MS VisualStudio
  IDE is used for Windows platforms.

  <<<< The windows port is not yet finished, please come back later! >>>>

**extern/**

  Any third-party sources which are compiled and added to the software
  bundle are here.
    
  **resources/**

    Contains the sources or tarballs of the different dependencies
        
  **<platform>/**

    Organized collection of the installed dependencies (acts as
    INSTALL_PREFIX)

If you want to compile all karabo projects as bundle consequently proceed:


1. From command-line (using make):

  .. code-block:: bash

    cd karaboFramework/build/netbeans/karabo
    make -j CONF=Debug bundle-install

  Be careful with the -j option, you may run out of memory if you use
  too many threads. For a release build choose CONF=Release.

2. From Netbeans (one possible way)

  * Start Netbeans
  * Open project: *karaboFramework/build/netbeans/karabo*
  * Build project
  * Open project: *karaboFramework/build/netbeans/karathon*
  * Build project

  In the karabo project navigate to the Makefile and run the target
  bundle-install

3. (Updated) If you fail during compilation of any of extern packages,
   please try to fix missing dependencies or other reason for errors
   and proceed with above command again. This file

  .. code-block:: bash

    karaboFramework/build/netbeans/karabo/.marker.txt

  contains list of all packages that are succesfully installed on your
  machine.


4. HINT: All bundle makefile targets will write into
   $HOME/.karabo/karaboFramework file the path to the current
   karaboFramework installation directory, which is used i.e. when
   compiling plugins.

  They will also go through all other projects (pythonGui, pythonCli,
  pythonKarabo, deviceServer and brokerMessageLogger) and compile and
  install them along with karabo library. In case of python projects,
  scripts are created and copied to installation directories along
  with python sources.

5. Finally you may want to update code asistance in net beans (see
   chapter :ref:`Code Assistance <netbeansCodeAssistance>`)


Creation of binary software bundle for shipping
===============================================

1. Create installer script including karabo libs and binaries and all
   external dependencies for shipping or for package developement:

  .. code-block:: bash

    cd karaboFramework/build/netbeans/karabo
    make bundle-package # or make package CONF=Release for Release configuration)

  * This will also go through all other projects as described above.

2. Create installer script without GUI:

  .. code-block:: bash

    cd karaboFramework/build/netbeans/karabo
    make package GUIOPT=NOGUI

3. The recommended way of compilation (or script creation and source
   copy in case of python projects) of pythonGui, pythonCli,
   pythonKarabo, deviceServer and brokerMessageLogger projects is
   through *bundle-install* and *bundle-package* targets described
   above. However you may also build C++ project like this (it will
   check for external and karabo library dependency automatically):

  * From command line:

    .. code-block:: bash

      cd karaboFramework/build/netbeans/deviceServer
      make -j4

  * From Netbeans

    * Open project: karaboFramework/build/netbeans/deviceServer
    * Build project
