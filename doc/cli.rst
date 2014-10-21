The GUI command line
====================

Upon clicking the IP[y]-button in the *Console* tab at the bottom
of the GUI, an `IPython <ipython.org>`_ command line opens. Here
you may simply type python commands.

One of the first commands to be typed is probably

::

    from karabo import *

as this loads most of the karabo functionality. If your goal is
to communicate with devices, you need a *DeviceClient*, which
you get by

::

    d = DeviceClient()

Once you have this device client, you may connect to a device.
This is done via

::

    dev = yield from d.getDevice("some_device")

You mentioned the *yield from*-clause? It tells the Python interpreter
to wait until the device actually answered. Hopefully your network
connection is fast, but if it is not, or the device does not respond
for a different reason, the GUI would stop responding until a reponse
is there, thus this technique.

Once we have a device, we can get or set its values, by typing code
like

::

    dev.currentSpeed
    dev.targetSpeed = 3

or we can call slots in a device, as in

::

    dev.start()
