..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _run/servers:

***********************
Starting device servers
***********************

Karabo servers manage the lifetime of Karabo devices. Once a server has
loaded a device class (as plugin) it can start and stop one or more instances
of that device and provide distributed (remote) access to it.

As Karabo provides three different APIs for device implementation 
also three different servers are needed (C++, Python, and Middle-Layer).

Karabo uses `daemontools <https://cr.yp.to/daemontools.html>`_ to
supervise all device servers. Their service directory is
``$KARABO/var/service``. Feel free to use daemontools directly,
modifying the service directory as you wish. If you don't want to
learn a new tool, there are some Karabo wrappers around daemontools
for common operations, which also have the advantage that they deduce
the service directory from ``$KARABO``, so you don't need to search
for it.

All commands have a ``-h`` parameter which gives you a help text. Most
commands are written such that they apply to all device servers,
unless you give a list of device servers on the command line. Here are
the wrappers:

karabo-check
  This prints the state of all device servers, whether and how long
  they are running or not. You should get suspicious if a device
  server has only been running for a very short time, which might
  indicate that it is crashing regularly.

  At typical output looks like this::

    karabo_projectDBServer: down 63 seconds
    karabo_guiServer: up 61 seconds
    karabo_dataLoggerManager: up 61 seconds, want down

  The ``want down`` in the third lines means that a TERM signal was
  sent to the ``karabo_dataLoggerManager``, but it did not terminate
  yet. You may use ``karabo-kill`` to send another signal.

karabo-start
  Start the device servers, and keep them running. Whenever one dies,
  it gets restarted.

karabo-stop
  stop the device servers, and don't restart them.

karabo-add-deviceserver
  Add a new device server. It takes the name of the new device server
  and the type as parameters, as well as additional options that
  should be added. There are three types: cppserver,
  middlelayerserver, and pythonserver.

  So if you want to add a cppserver named *myserver* with the option
  ``Logger.priority=INFO``, you would write::

    karabo-add-deviceserver myserver cppserver Logger.priority=INFO

  This should be sufficient for most usecases of device servers. If
  you need something special, feel free to change the ``run`` file in
  ``/var/service/myserver``. This way you may even do completely
  different things than running device servers, if you need to have
  something running all the time, just add a fake device server and
  edit its ``run`` file to your likings.

  The generated ``run`` file will read environment variables from
  ``$KARABO/var/environment``. Into this directory you should put any
  environment variable that you want to have set for all device
  servers in a Karabo installation. The name of the file is the name
  of the variable, its contents will be the value. Environment
  variables which should only be set for one server should better be
  added into the ``run`` file.

karabo-remove-deviceserver
  This stops the device server if necessary and removes it completely.

karabo-kill
  Sometimes devices servers may hang, and you need to kill them. With
  this command you can do that. As an example
  ``karabo-kill -k broken-server`` kills the broken server. It gets
  restarted unless it has been stopped with ``karabo-stop`` before.
  Don't forget that by default this command kills all device
  servers, so better list the ones you want to kill.

Despite some small differences, all servers share the following, most important
configurations:

1. **Message Broker URL**

   Every server needs a connection to the central broker. This address must be
   given in the format::
     
     tcp://<host>:<port>

   and can be set per shell using the environmental `KARABO_BROKER`.

   Example::
  
     export KARABO_BROKER=tcp://localhost:7777
  
   This tries to connect to a broker running locally on port 7777.

2. **Message Broker Topic**

   Each Karabo installation must use a single topic name under which all 
   devices and servers are talking with each other. Think about a topic
   like a chatroom on the server, only members of the same chatroom can share
   information.

   The topic can be provided as environmental as well, called 
   `KARABO_BROKER_TOPIC`.
   
   Example::
     
     export KARABO_BROKER_TOPIC=myTopic

3. **Server ID**

   Within a Karabo installation (i.e. under the same broker host and topic)
   each server needs a unique ID to be identified.

   As the server ID is specific to each server it must be provided as command
   line argument::

     karabo-<API>server serverId=MyServer
     
   With `<API>` being either `cpp`, `python` or `middlelayer`.

   The serverId can be skipped, in this case Karabo will generate a unique 
   (but cryptic) one, which will also be different for consecutive server starts.
   It is hence strongly recommended to *always* assign a serverId 
   
       
4. **Device Classes**

   Any server knows about all respective plugins (i.e. device classes) of its
   API type, which are installed to the Karabo framework on a given host. 
   Sometimes you want to explicitly steer which classes should be loaded by a given server.
   This can be achieved by an additional commandline argument::
     
     karabo-<API>server deviceClasses=MyDeviceClass

   or if several should be loaded::

     karabo-<API>server deviceClasses="MyDeviceClass1,MyDeviceClass2"
        

General notes
=============

Run Directory
-------------

Once Karabo is activated, servers can be started from anywhere in the filesystem.
The run-directory for servers however always is ``<path-to-karabo>/var/data``.
This for example means that a file created within a device process will be 
placed in ``<path-to-karabo/var/data`` if no different explicit path selector 
is provided.

Log Files
---------

Log-files are written out to ``std::out`` and to file. All logfiles are placed
in ``<path-to-karabo>/var/log``.

Processes vs. threads
---------------------

The C++ and Middlelayer servers run devices as threads/coroutines, whereas 
the (bound) Python server starts each device in an own process.
