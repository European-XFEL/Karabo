.. _installation-binary:

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

When asked for the respective paths, simply press Enter to use the defaults or just
enter a new path where you would like to install the Karabo Framework.

Fortunately, you do not have to install any dependencies as everything is shipped within Karabo.

Your installation of the Karabo Framework is now complete!

