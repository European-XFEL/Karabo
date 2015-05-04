.. _howto-cli:

********************************************
 How to use the command line interface (CLI)
********************************************

The interactive command line interface of Karabo uses
`Python 3 <http://docs.python.org/3>`_ as a language and
`IPython <http://www.ipython.org>`_ as interpreter.

It can be started in two ways: there is a command line built into the
GUI. Commands typed into there will be executed on the macro server,
which is a dedicated device for that purpose. Keep that into mind
when trying to open files: you cannot open files on your computer,
but only on the macro server.

The other way is to start ``ikarabo``, which is in the ``run/bin`` folder
of karabo. Here the code actually runs on the computer where it is
started, which means that you need to be able to connect to the Karabo
broker.

The commands are the same as for macros, see :ref:`howto-macro`.
In a typical session, one would normally start by connecting to a device::

    >>> d = connectDevice("some_device")

This gives a proxy of ``some_device`` with the name *d*. Note that in
macros we usually use :func:`getDevice`, to get a device and then connect
to it in a ``with`` statement. As this is cumbersome on the command line,
:func:`connectDevice` automatically connects to the device.

Many commands have tab-completion enabled, so once you type the tab key
on the above example just after you type the first quotation mark ("),
you will get a list of currently available devices.

Once we are connected, we can inspect or set properties, and call slots::

    >>> d.speed
    25
    >>> d.targetSpeed = 8
    >>> d.start()
    ['Started']
