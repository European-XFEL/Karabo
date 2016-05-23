Starting a full Karabo system all local
=======================================

Here we describe how you can bring up a small local Karabo system on your computer. Make sure you have understood the :ref:`fundamental concepts <fundamentals>` before proceeding here.

Lets go step by step. First of all you should navigate to the *karaboRun* folder
of your installion, e.g. as created during a :ref:`binary installation <installation-binary>`.


Step 1: Message Broker
----------------------

The first thing that must run for Karabo to work is the message broker. If you are in the European XFEL GmbH network you can use our always running broker (*exfl-broker:7777*) and skip to the next step. If you are outside of the European XFEL network you must start the broker locally on your computer. To accomplish this navigate into the *bin* folder and type::

  ./startBroker

This will start the broker as a background process. It will be reachable under *localhost:7777* from now on.

To stop the broker simply type::

  ./stopBroker

Step 2: Installing Devices
--------------------------

After installation, Karabo is still "empty". You first have to install some devices to do anything useful. We have a repository of devices that after downloading and installing will be available in Karabo.

Like the KaraboFramework itself, devices can be installed as binary packages or from sources. The binary installation typically is quicker and easier and you should use it if you do not want to further develop or bug-fix the devices (as no source code is shipped).

In either case, if you are dealing with devices, you should get familiar with the *karabo* script located in the top directory of *karaboRun*.

The script has an embedded help which you can see simpling executing it::

  ./karabo

NOTE: The first time you execute this script you are asked for your email address, which is only used if you create new devices.

Calling the script as above should produce an ouput like this::

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
    import    - Imports local project into repository
    
To get quickly going with Karabo try the demo configuration::

  ./karabo install-s karabo-demo

For that to work you must have read-access to our SVN repository, in case you don't, write a mail to karabo@xfel.eu and ask for it.

This command will check-out the devices (as defined in a file called 'karabo-demo' located in the *etc/* folder) to a folder called *package*/. 
C++ code is automatically compiled into a shared library. Subsequently, a soft-link to this library is created in the *servers/cppDeviceServer/plugins* folder such that on start-up the server will know about this device class.
If the device is coded in Python, the same thing will happen with the difference that *servers/pythonDeviceServer/plugins* is used (for the source file, as no compilation is needed).

The other subcommands are not documented here in detail, please use the script-internal help.

.. _run-startup:

Step 3: Starting up Karabo
--------------------------

Navigate to the *bin/* folder.

Before starting you have to tell Karabo which broker to use. There are two possibilities:

(a) By setting an environment variable::

      export KARABO_BROKER_HOSTS=localhost:7777

(b) By configuring it in the *allInfo* file. This file does not exist if you use Karabo the first time. In this case you
    create a copy of the *allInfo.orig* file, name it *allInfo*.
    With a text-editor open it and edit the KARABO_BROKER_HOSTS variable as under (a). By the way, in this file you can also configure, which device servers should be started.

Start Karabo::

  ./allStart

Stop Karabo::

  ./allStop

Or, in case you want to stop an individual server (here the cppDeviceServer), type::

  ./allStop cppDeviceServer

Use::

  ./allCheck

to see what Karabo applications are currently running.

You might want to learn more about these :ref:`commands to start and stop <run-scripts>` Karabo
in the following section.












