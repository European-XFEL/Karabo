.. _creating_devices:

*********************
Creating A New Device
*********************

New devices are created in Karabo using a script shipped as part of the
Karabo runtime environment. This script can be found in ./karaboRun and is
simply called ``karabo``.

Calling it without parameters yields a help screen from which further options
can be devices.

.. code-block:: bash

    ./karabo

    usage: ./karabo <subcommand> [options]

    Type './karabo help <subcommand>' for help on a specific command

    Available subcommands:

     install-b - Install a package configuration (download binary)
     install-s - Install a package configuration (build from sources)
     update    - Update (svn) installed packages
     rebuild   - Rebuild installed packages
     new       - Create a new package
     checkout  - Checkout an existing package (use list to retrieve name)
     list      - Create listings of various type
     tag       - Creates a svn tag for a given package
     branch    - Creates a svn branch for a given package
     setsvn    - Sets karabo svn repository path
     setrepo   - Sets url list to binary repositories
     import    - Imports local project into repository

Important for creating a new device is ``karabo new`` which creates a new
device package in svn.

.. code-block:: bash

    ./karabo new PACKAGE_NAME PACKAGE_CATEGORY TEMPLATE_TYPE TEMPLATE_NAME CLASS_NAME [options]

PACKAGE_NAME
    has to be a unique (alpha-numeric, camel case) name identifying a logical
    set of binaries/libraries.  Already existent packages can be listed using:
    ``./karabo list packages``.

A PACKAGE_CATEGORY
    has to be assigned to each package for organizational reasons
    Supported package categories can be listed using: ``./karabo list categories``
    A new category, if needed, can be added upon request.

TEMPLATE_TYPE
    is the type of the template to be used. Currently supported values are
    *application, cppDevice, pythonDevice, deviceServer* and *dependency*.
    Supported types can be listed using: ``./karabo list templates``.

TEMPLATE_NAME
    must be a valid template name under the specified template type.
    Possible templates can be listed using: ``./karabo list templates``.

CLASS_NAME
    ,in case of device, is the name of the class which will be initially
     generated. Use the convention for class capitalization,
    for example: TestMessageReader, LinkedList, LinearMotor. Do not use *Device*
    in the device class name, like LinearMotorDevice.
    In case of application it is the name of the source file containing
    the main function. Use camel case names for instance: tcpClientAsync,
    brokerMonitor.
    In case of a dependency, with template cppDep it is similar as for device.
    With custom template CLASS_NAME can be set simply the same as PACKAGE_NAME

Additional valid options are:

-f

    forces creation of package, i.e. will overwrite in case of (locally)
    pre-existing packages

-noSvn

    new package is not automatically integrated to the versioning system

Examples:

1. Create a package for a new CPP device. The name of device is set to simulatedCamera.

    .. code-block:: bash

        ./karabo new simulatedCamera testDevices cppDevice minimal SimulatedCamera

2. Create package for a new python device with minimal FSM. The name of the
    device is set to simulatedCamera.

    .. code-block:: bash

        ./karabo new simulatedCamera testDevices pythonDevice minimal SimulatedCamera

3. Create a package dependency myDep, which is in form of a Netbeans project.

    .. code-block:: bash

        ./karabo new myDep dependencies dependency cppDep MyDep

4. Create a package dependency, (e.g. matplotlib) which must be build using custom commands.

    .. code-block:: bash

        ./karabo new matplotlib dependencies dependency custom matplotlib

5. Create an application

    .. code-block:: bash

        ./karabo new myNewApp testApplications application defaultCpp myNewApp


Starting a development project using the ``karabo`` script
==========================================================

Start by creating a new device project using the ``karabo`` script and the
minimal pythonDevice template:

.. code-block:: shell

    $ # run karabo help new for a description of the parameters
    $ karabo new PACKAGE_NAME PACKAGE_CATEGORY pythonDevice minimal CLASS_NAME [-noSvn]

A pythonDevice project created from the template can be built in a couple of
different ways. The first way is by using the ``karabo`` script again:

.. code-block:: shell

    $ # Note that PACKAGE_NAME and PACKAGE_CATEGORY are the same as above
    $ karabo rebuild PACKAGE_NAME PACKAGE_CATEGORY

Building the device in this way automatically installs it to the
run/servers/pythonDeviceServer/plugins directory. If you would like to choose
where the device is installed, read below about the self-extracting shell script.

To build a redistributable self-extracting installer for a pythonDevice,
navigate to the device's source code directory and run the following command:

.. code-block:: shell

    $ ./build-package.sh

A self-extracting shell script will be saved by the build command. It's in a
deeply nested directory in the "package" directory in the device's directory.
Run this script to install the device at a location of your choice.

The third way to build a pythonDevice enables development of the device's code
without the need to reinstall after making changes to the code. To use this
method, you should first navigate to the device's source directory. Then run the
following command:

.. code-block:: shell

    $ ./build-package.sh develop

That will make a link to the device's source code directory so that it is
visible to the device server's plugin discovery code. Note that currently
running device servers will not immediately see a device installed in this way.
The test device server should be restarted after running the above command.
After restarting the server, further changes to the device's source code will be
immediately available without an installation step. You can simply instantiate
a new instance of the device to get the changes.
**You should be careful to stop any devices that were instantiated with older
versions of the code.**
Note that you will only see the results of changes in newly created device
instances and not in, for example, the configuration associated with the device
class.

When you are done developing the device, you should remove this link with the
following command:

.. code-block:: shell

    $ # The only difference is the "-u" argument at the end
    $ ./build-package.sh develop -u


Updating an older ``PythonDevice`` project
==========================================

If your device project was created from the pythonDevice minimal template but
it *doesn't* have a setup.py file (karaboFramework 1.3 and earlier), it can
be converted to the newer structure automatically. For this, you use the
``convert-karabo-device-project`` program which comes with a Karabo framework
installation:

.. code-block:: shell

    $ # Assuming the Karabo bin directories aren't in your path...
    $ $KARABO/extern/bin/convert-karabo-device-project <path-to-project>

The result of running this program is fairly straightforward:

* All Python source files in the project's 'src' directory are imported and
  checked for the presence of a subclass of ``PythonDevice``.
* All Python source files in the project's 'src' directory are moved to a new
  package directory which is created in the 'src' directory.
* A 'setup.py' file is added to the project's root directory. This file defines
  an entry point for each ``PythonDevice`` subclass that was found when scanning
  the project's sources.
* A current version of the 'build-package.sh' script is added to the project's
  root directory. The old 'build-package.sh' (if it exists) is moved to a file
  named 'build-package-old.sh'.

Once converted, the above instructions relating to invocation of the
'build-package.sh' script apply. Your device will build as a self-extracting
shell script when using the ``karabo`` script or if you like, you can build
in "development" mode too.


``setup.py`` and Device entry points
====================================

Starting with Karabo framework version 1.5.0, each Python device project should
use a ``setup.py`` script to package itself for installation on both developer
and user systems.

Exhaustive documentation for the ``setuptools`` library and ``setup.py``
scripts can be found `here <https://pythonhosted.org/setuptools/setuptools.html>`_

To start, here is a sample ``setup.py`` script from a project which contains a
single device:

.. code-block:: python

    #!/usr/bin/env python

    from setuptools import setup, find_packages

    long_description = """\
    Surrounded by rocky, lifeless worlds and in need of a quick place to land
    your ship? Never fear! The Genesis Device is for you!

    * WARNING: Not to be used on inhabited planets. Point away from face when
    using. May cause grey goo.
    """

    setup(name='genesisDevice',
          version='1.0.5',
          author='Joe Smith',
          author_email='joe.smith@xfel.eu',
          description='Genesis Device: Rapid Planet Terraformer',
          long_description=long_description,
          url='http://en.memory-alpha.wikia.com/wiki/Genesis_Device',
          package_dir={'': 'src'},
          packages=find_packages('src'),
          entry_points={
              'karabo.python_device.api': [
                  'Genesis = genesisDevice.Genesis:GenesisTorpedo',
              ],
          },
          package_data={'': ['*.dat']},
          requires=['roddenberry >= 1.0'],
          )

The ``setup.py`` really only needs to call the ``setup`` function provided by
``setuptools``. For more complicated packages, C-API modules can be compiled or
versioning schemes can be implemented in the ``setup.py`` script. For most
Karabo devices, this simple example should be sufficient.

The most important keyword arguments are ``name``, ``packages``, and
``entry_points``.

``name`` is the name of the package. This should be obvious.

``packages`` is a list of all the Python packages that are part of this project.
For a simple device, this list might only have a single item. In this example,
that would be ``['genesisDevice']``. For more complicated projects, this list
should be a complete package hierarchy. For instance:
``['genesisDevice', 'genesisDevice.subPackage', 'genesisDevice.otherSub']``
would describe a Python package with two subpackages. The ``find_packages``
function provided by ``setuptools`` handles the creation of this package list
easily. In the case of a project based on the pythonDevice minimal template, the
packages are just directories contained within the 'src' directory which are
themselves Python packages (ie: They contain an ``__init__.py`` file).

``entry_points`` is a dictionary of classes which can be loaded by a device
server. The key used here is ``'karabo.python_device.api'``, which specifies
devices using the C++ like API. For the Pythonic API, the key is
``'karabo.python_device.api_2'``. The value is a list of strings which describe
the individual device entry points. The basic format is:

.. code-block:: python

    'UNIQUE_NAME = PACKAGE.[SUBPACKAGE.SUBPACKAGE.]SUBMODULE:CLASS_NAME'

``UNIQUE_NAME`` is some unique identifier for the device. After the equal-sign,
a path to the device's class is given. You can think of it as something like an
``import`` statement. The equivalent for the example would be:

.. code-block:: python

    from genesisDevice.Genesis import GenesisTorpedo

When the device server is running, it periodically checks its namespace
(api or api_2) for all available device entry points. It attempts to import
each device. Every device which can be imported and which is a subclass of
``PythonDevice`` will be made available for instantiation by the server.

Some other potentially useful keyword arguments for the ``setup`` function are
``package_data`` and ``requires``. ``package_data`` is a dictionary of file
globs which allows for inclusion of non-Python sources in a built package.
``requires`` is a list of strings which denote third-party Python packages
which are required for the device to run. These arguments and others are
explained more completely in the ``setuptools``
`documentation <https://pythonhosted.org/setuptools/setuptools.html>`_


Documenting Devices
===================

Running ``./karabo new`` will also create a template for the device documentation
in the docs folder of the device. Documentation in Karabo is written in
restructured text. Minimally, a device which is to be deployed for production
usage needs to document its state diagram, and known error conditions that
might occur.