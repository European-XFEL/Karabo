.. _installation-binary:

**************************
 Binary download for Linux
**************************

Use this installation option if you want to use Karabo for:

- Running the GUI or ikarabo on your Laptop or PC in order to view/control an existing installation

- Starting Karabo all local, see how it works, develop macros and/or devices

- Build up small control systems involving few computers

- Developing devices or macros for Karabo

If one of the option is for you simply follow the few steps below:


Get and install Karabo Framework
===================================

A self extracting shell script is available `here <http:exflserv05.desy.de/karabo/karaboFramework/tags>`_.

Select the correct installer for your operating system and download it to some local folder.

Change permissions and extract it using the following commands::

    chmod +x karabo-*.sh
    ./karabo-*.sh

The script will install a single ``karabo`` folder, containing everything needed (including dependencies)
to get going with Karabo.

.. note::

   Before using Karabo you always have to source the ``activate`` script which is
   available in Karabo's root folder.

Once ``activate`` is sourced, your environment is ready and you should be able to e.g. run:

``karabo-gui``



