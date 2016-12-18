.. _creating_devices:

*********************
Creating A New Device
*********************

New devices are created in Karabo using a script shipped as part of the
Karabo runtime environment. The script is simply called ``karabo``.

Calling it without parameters yields a help screen from which further options
can be found.

Important for creating a new device is ``karabo new`` which creates a new device
package in the *devices/* folder of karabo. The device code is already versioned
using a local git repository. Once the device is deemed useful for maintaining
it centrally it can simply be pushed to the remote git repository:

.. code-block:: shell

    $ git push -u origin master 
  
.. note::

    In the current implementation an empty project in gitlab must be created 
    prior to pushing. Ask the CAS team if you need assistence.

Starting a development project using the ``karabo`` script
==========================================================

Start by creating a new device project using the ``karabo`` script and the
minimal pythonDevice template:

.. code-block:: shell

    $ # run karabo --help new for a description of the parameters
    $ karabo new PACKAGE_NAME python minimal

A pythonDevice project created from the template can be built in a couple of
different ways. The first way is by using the ``karabo`` script again:

.. code-block:: shell

    $ # Note that PACKAGE_NAME is the same as above
    $ karabo develop PACKAGE_NAME

Building the device in this way automatically installs it (using **pip**) into
Python's site-packages (the one shipped with Karabo).

.. todo::

   This chapter is largely incomplete, more documenation has to be added.




Documenting Devices
===================

Running ``./karabo new`` will also create a template for the device documentation
in the docs folder of the device. Documentation in Karabo is written in
restructured text. Minimally, a device which is to be deployed for production
usage needs to document its state diagram, and known error conditions that
might occur.
