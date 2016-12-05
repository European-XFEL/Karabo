***************************
__CLASS_NAME__ Device (C++)
***************************

Compiling
=========

1. In the device's top directory simply type:

    ``make``

    Optionally specify ``CONF=Debug`` or ``CONF=Release`` for a debug
    or release build (default: Debug).


2. For coding/compiling using NetBeans, open the device's top directory
   as a NetBeans project and press the compile button.

Testing
=======

After a successfull build, a shared library is generated here:

``dist/<configuration>/<system>/lib__PACKAGE_NAME__.so``


The library must be added to the ``plugins`` folder of your
Karabo installation for any device-server to be able to see this device.

You may create a soft-link to the ``lib__PACKAGE_NAME__.so`` file in the
``plugins`` folder manually, or simply use the karabo script:

``karabo develop __PACKAGE_NAME__``

Running
=======

If you want to manually start a server using this device, simply type:

``karabo-cppserver serverId=cppServer/1 devices=__CLASS_NAME__``

Or just use (a properly configured):

``karabo-start``

Packaging
=========

A binary installation package can be created by typing:

``make package``
