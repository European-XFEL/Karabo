**************
__CLASS_NAME__
**************

Compilation
===========

1. Navigate to the build directory and type

    ``make``

    Optionally specify ``CONF=Debug`` or ``CONF=Release`` for a debug
    or release build (default: Debug)


2. For coding/compiling using netbeans, open the folder 
containing this README as netbeans project

Testing
=======

After building a shared library is generated here:

``dist/<configuration>/<system>/lib__PACKAGE_NAME__.so``

The library has to be added to the ``plugins`` folder of your
Karabo installation for any device-server to be able to see this device.

You may create a soft-link to the ``lib__PACKAGE_NAME__.so`` file in the
``plugins`` folder manually. Another, more convenient way is to use the 
``karabo`` utility script, simply type:

``karabo develop __PACKAGE_NAME__``

Running
=======

If you want to manually start a server using this device, simply type:

``karabo-cppdeviceserver serverId=cppServer/1 devices=__CLASS_NAME__``




