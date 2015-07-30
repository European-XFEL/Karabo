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
* Each TCP message encodes one Hash which must have the key "type"

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

*instanceNew*
topologyEntry (Hash)

*instanceUpdated*
topologyEntry (Hash)

*instanceGone*
instanceId (string)
instanceType (string)

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

*NOTE: The names in brackets are the new protocol names and will deprecate over time the ones without brackets*
