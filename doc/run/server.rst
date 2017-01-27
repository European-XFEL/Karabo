.. _run/servers:

***********************
Starting device servers
***********************

Karabo servers manage the lifetime of Karabo devices. Once a server has
loaded a device class (as plugin) it can start and stop one or more instances
of that device and provide distributed (remote) access to it.

As Karabo provides three different APIs for device implementation 
also three different servers are needed (C++, Python, and Middle-Layer).

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






