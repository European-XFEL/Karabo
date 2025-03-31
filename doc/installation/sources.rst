..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

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

    git clone https://git.xfel.eu/Karabo/Framework.git karaboFramework
    cd karaboFramework

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


2.  If there were only code changes then simply rebuild:

  .. code-block:: bash

    ./auto_build_all.sh Debug

  or recompile in the IDE you are using.



Install dependencies
====================

Supported Operating Systems
---------------------------

The operating systems which are currently officially supported by Karabo are: Centos 7,
Ubuntu 20.04, and 22.04, Debian 10, and AlmaLinux 9. To learn which
packages must be installed before building Karabo, refer to the following
project on the XFEL GitLab server:

https://git.xfel.eu/Karabo/ci-containers/

There you will find the following Dockerfiles which list the packages needed
on each platform.

https://git.xfel.eu/Karabo/ci-containers/-/blob/master/centos/7gcc7/Dockerfile

https://git.xfel.eu/Karabo/ci-containers/-/blob/master/ubuntu/20.04/Dockerfile

https://git.xfel.eu/Karabo/ci-containers/-/blob/master/ubuntu/22.04/Dockerfile

https://git.xfel.eu/Karabo/ci-containers/-/blob/master/debian/10/Dockerfile

https://git.xfel.eu/Karabo/ci-containers/-/blob/master/almalinux/9/Dockerfile

Framework/-/merge_requests


These same files are used to generate the continuous integration infrastructure
for Karabo, so they are more up to date than any documentation can hope to be.

Even though these are docker scripts, they are quite simple and consist mainly
of ``apt-get install``, ``yum install`` or ``dnf install`` commands
(depending on the platform).

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

To run one of the Framework unit or integration tests from the command line,
please open a terminal, go to your build directory and issue a `ctest`
command. In the example below, all tests are run in the default non verbose
mode (please note that ``build_debug`` is specific to this example; for Release
builds from `auto_build_all.sh`, for instance, it would be `build_release`)::

.. code-block:: bash

  cd $REPO_ROOT/build_debug
  source ./activateKarabo.sh
  ctest

In the example above, $REPO_ROOT is the directory where you have git cloned the
Karabo Framework repository (the directory where file `auto_build_all.sh` is).
The script `activateKarabo.sh` is a subset of the `activate` script found in a
full blown Karabo installation and handles the configurations needed to be able
to run the C++ unit and integration tests directly from the C++ build tree of
the Framework.

It is also possible to run all tests whose names match a given regular expression
in either verbose mode (`-V` option) or extra verbose mode (`-VV`). In the example
below, `dataLoggingIntegrTestRunner` is the only test run, and in extra verbose
mode::

.. code-block:: bash

  ctest -VV -R "dataLogging*"

Verbose and extra verbose modes cause `ctest` to output, among other things,
one line per successful test case execution. The default verbosity
level only emits intermediate reports for failed test cases. The number (but not
names) of successful test cases is printed at the end of the `ctest` run, when the
default verbosity level is used.

To list all the tests that are available for `ctest`::

.. code-block:: bash

  ctest -N

`ctest` also supports a `-E` option which is the complement of the `-R` option,
meaning execute all tests that do not match the given regular expression.

For instructions on how to run the C++ tests from the Visual Studio Code IDE,
please refer to
`this section of the related documentation <https://karabo.pages.xfel.eu/Framework/tools/vscode.html#run-and-debug-the-framework-tests>`_.


PythonKarabo (Python)
---------------------

To test Python code be aware that if you depend on Karathon (and
Karabo C++ code) you must install and deploy the changes you may have
done in Karabo/Karathon in your system.

In a Terminal session, you can do that by running:

.. code-block:: bash

  # $REPO_ROOT is the directory where the local working copy of the Karabo Framework
  # repository is.
  cd $REPO_ROOT

  ./auto_build_all.sh Debug/Release

To run the Unit Tests using the Terminal, execute the following scripts:

.. code-block:: bash

  source $REPO_ROOT/karabo/activate

  cd $REPO_ROOT/src/pythonKarabo

  # This will run all the Python tests.
  pytest -v karabo 


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


**extern/**

  Any third-party sources which are compiled and added to the software
  bundle are here.

  **resources/**

    Contains the sources and build configurations of the different external
    dependencies

  **<platform>/**

    Organized collection of the installed dependencies (acts as
    INSTALL_PREFIX)


Creation of binary software bundle for shipping
===============================================

Create installer script including karabo libs and binaries and all
external dependencies for shipping or for package developement:

  .. code-block:: bash

     ./auto_build_install.sh Release --bundle

  After successfull bundling you should find a ``karabo-<version>.sh`` in

  ``package/<Conf>/<OS-Name>/<OS-Version>/<Arch>/``
