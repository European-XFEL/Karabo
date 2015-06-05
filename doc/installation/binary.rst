**************************
 Binary download for Linux
**************************

Use this installation option if you want to use Karabo for:

- Running the GUI or CLI on your Laptop or PC in order to view/control an existing installation

- Starting Karabo all local, see how it works, develop macros and/or devices

- Build up small control systems involving few computers

- Developing devices or macros for Karabo

If one of the option is for you simply follow the few steps below:


Get and install Karabo Framework
===================================

A self extracting shell script is available `here <ftp://karabo:framework@ftp.desy.de/karaboFramework/tags>`_.

Select the correct installer for your operating system and download it to some local folder.

Change permissions and extract it using the following commands::

    chmod +x karabo-*.sh
    ./karabo-*.sh

The script will install two folders, one for the Karabo framework (*karabo-<version>*), containing libraries, dependencies, etc., and another one for running Karabo (*karaboRun*).

When asked for the respective pathes, simply press Enter to use the defaults or just
enter a new path where you would like to install the Karabo Framework.

Fortunately, you do not have to install any dependencies as everything is shipped within Karabo.

Your installation of the Karabo Framework is now complete!

Starting the GUI or CLI
==========================

Navigate to the *bin* folder in karaboRun and type::

  ./startGui

or::

  ./startCli

depending on what you want.

Instructions of how to use the GUI can be found :ref:`here <howto-gui>`.

Using the CLI is described :ref:`here <howto-cli>`.

If you log into the correct server you will be immediately able to view and control (given your access persmissions allow it) devices that are currently online.


Starting Karabo all local
=========================

Here we describe how you can bring up a small local Karabo system on your computer. Make sure you have understood the :ref:`fundamental concepts <fundamentals>` before proceeding here.

Lets go step by step. First of all you should navigate to the *karaboRun* folder you just installed.

Step 1: Message Broker
----------------------

The first thing that must run for Karabo to work is the message broker. If you are in the European XFEL GmbH network you can use our always running broker (*exfl-broker:7777*) and skip to the next step. If you are outside of the European XFEL network you must start the broker locally on your computer. To accomplish this navigate into the *bin* folder and type::

  ./startBroker

This will start the broker as a background process. It will be reachable under *localhost:7777* from now on.

To stop the broker simply type::

  ./stopBroker

Step 2: Installing Devices
--------------------------

After installation, Karabo is still "empty". You first have to install some devices to do anything useful. You can think about devices like the apps on your smartphone. We have a repository of devices that after downloading and installing will be available in Karabo.

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
    






