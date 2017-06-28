***************
Debugging - GDB
***************

This document describes how to use GDB with your C++ devices, on a localhost.
This document is a work in progress.

.. note::
        Note that you are debugging a distributed system, and that the behaviour
        of various components (brokers, servers, UI clients...) may be affected
        in unforseen ways.

Setup
+++++

The following are required:
 * GDB
 * A working installatio of Karabo
 * Device compiled with debug flags (*karabo develop DeviceName*)
 * sudo rights

Getting Started
+++++++++++++++
Getting from zero to a breakpoint requires 6 steps.

Start the cppserver::

    $ karabo-cppserver serverId=gdbServer/1 [devices=]

Initialise the device under test in karabo-gui

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

Remember to continue once you're ready, by inputting `c`. Also, this is a
distributed system, and the resources you are inspecting may be needed by other
components, and as such affect their behaviour.



Known Issues
++++++++++++
If too much time is spent in the suspended state (ie. stopped on a breakpoint),
then the cppserver will crash and will require a `kill -9`. This could be due to
the broker dropping the connection when the device is irresponsive for a period
of time. Consider using watchpoints instead.

To Do:
++++++
The following are suggestions to improve debugging:
 * Get a graphical client (Netbeans integration, gdbgui or...)
 * See whether the gdbserver can be bypassed altogether

.. note::
    You are debugging a distributed system
