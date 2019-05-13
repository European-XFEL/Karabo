.. _guiServer:

**********
Gui Server
**********

GuiServerDevice
===============

Concept
^^^^^^^

* The GuiServerDevice on the one hand is connected to one broker (under specific host, port and topic) and on the other hand to zero or more clients
* Hence, clients can connect to Karabo only indirectly via the server 
* Communication is entirely event-driven
* The server is powered by a DeviceClient, it thus performs caching
* Communication overhead is reduced by informing the server about actually "visible" devices who only receive updates

Protocol
^^^^^^^^

* The communication protocol uses the binary serialized Hash
* Each TCP message encodes one Hash which must have the key "type". Depending
on the type, other keys are incuded as documented below.

**Message types OUT (send from the server to the client):**

*brokerInformation*
host (string)
port (unsigned int)
topic (string)

*systemTopology*
systemTopology (Hash)

*configurationChanged (deviceConfiguration)*
deviceId (string)
configuration (Hash)

*deviceSchema*
deviceId (string)
schema (Schema)

*classSchema*
serverId (string)
classId (string)
schema (Schema)

*propertyHistory*
deviceId (string)
property (string)
data (vector<Hash>)

[depr]_ *instanceNew*
topologyEntry (Hash)

[depr]_ *instanceUpdated*
topologyEntry (Hash)

[depr]_ *instanceGone*
instanceId (string)
instanceType (string)

[2.5.0]_ *topologyUpdate*
topologyUpdates (Hash)

This Hash groups information about updates to the topology, i.e. "new", "gone" and "update"d instances.
These three first level keys are always present. One level down is the instance type (e.g. device, server,...)
and at the final third level the keys are instanceIds. For the "new" and "update"
case, attributes carry their "instance info".
An example Hash of the ``topologyUpdates`` is shown below.

::

     'type' => topologyUpdate STRING
     'changes' +
       'new' +
         'device' +
           'DataLogger-clog_0' type="device" classId="DataLogger" serverId="karabo/dataLogger" visibility="4" compatibility="1.0" host="exflqr30450" status="ok" archive="0" capabilities="0" heartbeatInterval="60" KaraboVersion="3913949" p2p_connection="tcp://exflqr30450.desy.de:60517" +
           'DataLogger-Karabo_AlarmService' type="device" classId="DataLogger" serverId="karabo/dataLogger" visibility="4" compatibility="1.0" host="exflqr30450" status="ok" archive="0" capabilities="0" heartbeatInterval="60" karaboVersion="3913949" p2p_connection="tcp://exflqr30450.desy.de:60517" +
       'update' +
         'device' +
           'cppServer/1_PropertyTest' type="device" classId="PropertyTest" serverId="cppServer/1" visibility="4" compatibility="1.0" host="exflqr30450" status="ok" archive="1" capabilities="0" heartbeatInterval="120" karaboVersion="3913949" p2p_connection="tcp://exflqr30450.desy.de:60517" +
       'gone' +
         'server' +
           'karabo/macroServer' +


*notification*
deviceId
messsageType (string)
shortMsg (string)
detailedMsg (string)

*log*
message (string)

**Message types IN (send from the client to the server):**

*login*
username (string)
sessionToken (string)
provider (string)

*reconfigure*
deviceId (string)
configuration (Hash)
  
*execute*
deviceId (string)
command (string)
  
*refreshInstance (getDeviceConfiguration)*
deviceId (string)

*getDeviceSchema*
deviceId (string)

*getClassSchema*
serverId (string)
classId (string)

*initDevice*
serverId (string)
classId (string)
deviceId (string)
configuration (Hash)

*killServer*
serverId (string)

*killDevice*
deviceId (string)

*newVisibleDevice (startMonitoringDevice)*
deviceId (string)

*removeVisibleDevice (stopMonitoringDevice)*
deviceId (string)

*getFromPast (getPropertyHistory)*
deviceId (string)
property (string)
t0 (string ISO format)
t1 (string ISO format)
maxNumData (int)

*error*
traceback (string)

.. rubric:: Footnotes
.. [depr] Deprecated in Karabo 2.5.0: GUI client shall still understand them to connect to older GUI Server versions. GUI client legacy support will be dropped in 2.6.0.
.. [2.5.0] Introduced in Karabo 2.5.0 to replace *instanceNew*, *instanceUpdated* and *instanceGone*.

