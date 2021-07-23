.. _installation/sources:

******************
Build from sources
******************

Get and install the Karabo framework (quick - no details)
=========================================================

1. Create or move into a folder where you want to check out the 
   Karabo Framework, e.g.

  .. code-block:: bash

    mkdir Development
    cd Development

2. Now check out:

  .. code-block:: bash

    git clone https://in.xfel.eu/gitlab/Karabo/Framework.git karaboFramework

3. Compile the Karabo bundle by running the following command for the
   Debug version

  .. code-block:: bash

    ./auto_build_all.sh Debug

  Or this following one for the Release version, respectively.

  .. code-block:: bash

    ./auto_build_all.sh Release

  .. note::
  
     The compilation will only succeed if you installed all of the base
     dependencies. You must do this manually (see below)

4. When using a supported platform (and running from inside the DESY network),
   external dependencies are downloaded and installed rather than built locally
   on your computer. If this is the case, you can skip to the next step.
   
   When the above is not the case, there is a long (ca. 1 hour) build of the
   external dependencies. If part of this failed to build for some reason, try
   and fix the error in the dependency and compile again. For reference,
   this file:

  .. code-block:: bash

    karaboFramework/extern/<platform>/.marker.txt

  contains a list of packages which have already been built successfully on your
  machine.

  To build a single dependency (when, for instance, debugging a build failure),
  the following command can be used:

  .. code-block:: bash

    cd extern
    ./build.sh <platfowm> package0 [... packageN]


5. Before running the freshly compiled Karabo, you have to activate it:

  .. code-block:: bash

     source ./karabo/activate

  Each time you start a new shell, you will need to activate Karabo before
  attempting to run it. (If you don't, an error message will kindly remind you)

  To undo the environment parameter settings introduced by activation, just do:

  .. code-block:: bash

     deactivate

Tips for re-compilation of karaboFramework if you have already your
local working copy.

1. If there were any extern updates (for instance a new package, or a
   new version of existing package)

  * the very clean way is to rebuild all extern with:

    .. code-block:: bash

      ./auto_build_all.sh Clean-All
      ./auto_build_all.sh Debug


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



Install dependencies
====================

Supported Operating Systems
---------------------------

The operating systems which are currently supported by Karabo are: Centos 7,
Ubuntu 16.04, and Ubuntu 18.04 (and generally Ubuntu 16-19). To learn which
packages must be installed before building Karabo, refer to the following
project on the XFEL GitLab server:

https://in.xfel.eu/gitlab/Karabo/ci-containers/

There you will find the following Dockerfiles which list the packages needed
on each platform.

https://in.xfel.eu/gitlab/Karabo/ci-containers/blob/master/centos/7/Dockerfile

https://in.xfel.eu/gitlab/Karabo/ci-containers/blob/master/ubuntu/16.04/Dockerfile

https://in.xfel.eu/gitlab/Karabo/ci-containers/blob/master/ubuntu/18.04/Dockerfile

These same files are used to generate the continuous integration infrastructure
for Karabo, so they are more up to date than any documentation can hope to be.

Even though these are docker scripts, they are quite simple and consist mainly
of ``apt-get install`` or ``yum install`` commands (depending on the platform).

In addition, in order to run the system in completely local mode or run unit tests,
the host should have a working installation of ``Docker`` for the user that will run
Karabo. Please refer to the install page of `Docker <https://docs.docker.com/install/>`_.


Executing Unit Tests
====================

Besides regular unit tests, testing Karabo includes also more advanced
integration tests.

The simplest way to run all tests is:

.. code-block:: bash

  ./auto_build_all.sh Debug --runTests --runIntegrationTests
  

Karabo (C++)
------------

To run the Karabo unit tests please guarantee your local changes are
compiled (via Terminal or Netbeans).
 
To run the unit tests using the Terminal, please go to the
installed karaboFramework folder and execute the following scripts:

.. code-block:: bash

  cd build/netbeans/karabo
  make test

To run the integration unit tests, do the following:

.. code-block:: bash

  cd build/netbeans/integrationTests
  make test

To run the tests using Netbeans:

* Go to Karabo project (for the unit tests) or to integrationTests project
  (for the extended tests)
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
 
To run the Unit Tests using the Terminal, execute the following scripts:

.. code-block:: bash

  # This will run ZERO tests if you are in the framework root directory
  nosetests-3.4 -sv karabo  # or karabo.bound_api or karabo.middlelayer_api or karabo.tests, etc.
 
To run the Unit Tests using Netbeans:

#. Go to Tools > Python Platforms
#. Make as Default Python the Python under your current KaraboFramework
   installation

  #. Select "New"

  #. Add python available on your current installation extern folder
     (i.e. /.../your_current_karaboFramework/package/Debug/Ubuntu/14.04/x86_64/karabo/extern/bin/python)

  #. Choose new Python and make it default, selecting "Make Default"

  #. Select Close

#. Run Python Karabo project


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

  **integrationTests/**
  
    Contains code for high-level integration tests.

  **karabo/**

    The central project, Karabo's backbone in C++. Its directory
    structure is reflected into the namespaces and include hierarchy.

  **karathon/**

    C++ binding layer to make karabo available to the Python
    programming language.

  **pythonGui/**

    Native python code using PyQt4 and karathon to implement the
    graphical user interface of Karabo.

  **pythonKarabo/**

    Native Python code providing two APIs: The middlelayer API which is pure
    Python and the bound API which makes use of the bindings to Karabo's C++ API
    provided by karathon.

  **templates/**

    Here the templates for Karabo's three API's are placed 
    (will be utilized upon ``karabo new [...]``)

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

**extern/**

  Any third-party sources which are compiled and added to the software
  bundle are here.
    
  **resources/**

    Contains the sources and build configurations of the different external
    dependencies
        
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

     ./auto_build_install.sh Release --bundle
  
  After successfull bundling you should find a ``karabo-<version>.sh`` in 

  ``package/<Conf>/<OS-Name>/<OS-Version>/<Arch>/``

2. Create installer script without GUI:

  .. code-block:: bash

    cd karaboFramework/build/netbeans/karabo
    make package GUIOPT=NOGUI
