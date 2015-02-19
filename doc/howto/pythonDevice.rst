
*********************************
 How to write a device in Python
*********************************





C++ like API based on Python bindings
=====================================


Pythonic API based on native Python
===================================

A device is not much more than a macro that runs on a server for a longer
time. So it is written mostly in the same way. The biggest difference
is that it inherits from :class:`karabo.device.Device` instead of
:class:`karabo.device.Macro`. But the main difference is actually that
a macro is something you may write quick&dirty, while a device should be
written with more care. To give an example:

::

    from karabo import Device

    class TestDevice(Device):
        __version__ = "1.3 1.4"

As you see, we avoid using star-imports but actually import everything by
name. As the next thing there is a *__version__* string. This is not the
version of your device, but the Karabo versions your device is supposedly
compatible to.
