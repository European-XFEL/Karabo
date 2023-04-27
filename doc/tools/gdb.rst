..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

UPDATE: As of 2022, use of Netbeans for doing C++ development both for the
Karabo Framework and for Karabo Devices is being phased out. The primarily
supported C++ IDE is Visual Studio Code. Visual Studio Code offers excelent
support for both local and remote debugging. Please refer to the documentation
at https://code.visualstudio.com/docs/remote/remote-overview.

***************
Debugging - GDB
***************

This document describes how to use GDB with your C++ devices, locally and
remotely.

.. note::
        Note that you are debugging a distributed system, and that the behaviour
        of various components (brokers, servers, UI clients...) may be affected
        in unforseen ways.

Setup
+++++

The following setup is required:
 * GDB
 * A working installation of Karabo
 * sudo rights
 * A device compiled with debug flags:
    - Use the *karabo develop DeviceName* command
    - Compile from Netbeans in *Debug* configuration, not *Release*
    - Use *make CONF=Debug*

Debugging Locally
+++++++++++++++++
To debug a device running locally on your machine, you may start the cppserver
from within GDB::

    $ gdb karabo-cppserver

At this stage, set a breakpoint to the file and line you are interested in::

    (gdb) b HandsOnCpp.cc:112

A warning message will be displayed, as the server has not been started yet, and
the libraries are not loaded, hit `y` to enable the breakpoint::

    No symbol table is loaded.  Use the "file" command.
    Make breakpoint pending on future shared library load? (y or [n]) y

Now, start the server from within GDB::

    (gdb) run serverId=gdbServer/1 [devicesClasses=...]

Now create the device to inspect. This can be done in two different ways:

 * From karabo-gui: use the *Instantiate Device* button in the *Configuration Editor* Panel
 * From ikarabo::

    instantiate("gdbServer/1",
                "HandsOnCpp",
                "instance_id",
                <configuration_hash>)


The prompt will return once the breakpoint is hit, and you may then inspect
variables::

   Thread 9 "karabo-cppserve" hit Breakpoint 1, karabo::HandsOnCpp::produce
   (this=0x7fffd400deb0, e=...) at src/HandsOnCpp.cc:112
   112          if(newCount < -10){
   (gdb) print newCount
   $1 = 7
   (gdb)

Continue the process by inputting `c`.

Debugging Remotely
++++++++++++++++++
Getting from zero to a breakpoint requires 6 steps.

Start the cppserver::

    $ karabo-cppserver serverId=gdbServer/1 [devicesClasses=...]

Initialise the device under test in karabo, as described above.

Get the pid of this server, in this case 1906::

    $ ps aux | grep '[k]arabo-cppserver serverId=gdbServer/1' | awk '{print $2}'
    1906

Hijack the cppserver process with GDB::

    $ sudo gdbserver --attach :1234 1906

This will then start a gdb server on localhost on port 1234. This allows for
remote debugging, but then keep in mind the added complexity.

Start your GDB session::

    $ gdb

    [Legalese]
    Type "apropos word" to search for commands related to "word".
    (gdb) target remote :1234


Now add a breakpoint to the file and line you are interested in::

    (gdb) b HandsOnCpp.cc:112
    Breakpoint 1 at 0xFFFFFFF: file src/HandsOnCpp.cc, line 112.
    (gdb) c
    Continuing.

And finally, continue, the gdb prompt will return once the breakpoint is hit,
and you may then inspect variables::

    Thread 7 "karabo-cppserve" hit Breakpoint 1, karabo::HandsOnCpp::produce (
            this=0x7fe56001d470, e=...) at src/HandsOnCpp.cc:112
    112         if(newCount < -10){
    (gdb) print newCount
    $1 = 7
    (gdb)

Continue once you're ready, by inputting `c`.


Known Issues
++++++++++++
If too much time is spent in the suspended state (ie. stopped on a breakpoint),
then the cppserver will crash and will require a `kill -9`. This could be due to
the broker dropping the connection when the device is irresponsive for a period
of time. Consider using watchpoints instead.

Future work
+++++++++++
The following are suggestions to improve debugging:
 * Get a graphical client (Netbeans integration, gdbgui or...)

.. note::
    You are debugging a distributed system, the resources you are inspecting may
    be needed by other components, and as such affect their behaviour.
