..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

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
topic (string)
hostname (string)
hostport (unsigned int)
deviceId (string - ID of the GuiServerDevice)
readOnly (bool - the GUI Server only allow read-only operations)
version (string - version of the GUI Server)
authServer (string - URL of the Karabo Authentication Server)
[2.20.0]_ allowRememberLogin (bool - absent if authServer is not defined)

*systemTopology*
systemTopology (Hash)

[2.20.0]_ *onEscalationExpired*
expiredToken (string),
expirationTime (string),
levelBeforeEscalation (int - matching a karabo::util::Schema::AccessLevel value),
loggedUserId (string)

* Message sent by the GUI Server to the client whose privilege escalation has expired. The GUI Server checks for expired escalations every 10 seconds; most likely there will be a delay between the expiration and the client being communicated about it.
* The *expirationTime* string is the UTC date and time of the expiration in ISO-8601 format, e.g, "20240205T120633.139529Z".

[2.20.0]_ *onEndEscalationNotice*
aboutToExpireToken (string),
secondsToExpiration (unsigned int 64)

* Message sent by the GUI Server to the client whose privilege escalation is about to expire. The GUI Server checks for escalations lifetimes every 10 seconds.
* An escalation is considered "about to expire" if its expiration time is less than "endEscalationNoticeTime" seconds away. "endEscalationNoticeTime" is an init-only property of the GUI Server.

[2.20.0]_ *onEscalate*
success (bool),
reason (string),
accessLevel (int - matching a karabo::util::Schema::AccessLevel value),
escalationDurationSecs (int)

* Message sent by the GUI Server with the results of an escalation to the requesting client.
* For successful escalations, *success* will be *true*, *reason* will be empty, *accessLevel* will be the access level escalated to, and *escalationDurationSecs* will be the lifetime of the escalation in seconds.
* For rejected escalations, *success* will be *false* and *reason* will be a message describing the reason for the rejection.

[2.20.0]_ *onDeescalate*
success (bool),
reason (string),
levelBeforeEscalation (int - matching a karabo::util::Schema::AccessLevel value),
loggedUserId (string)

* Message sent by the GUI Server with the results of a deescalation to the resquesting client.
* For successful deescalations, *success* will be *true*, *reason* will be empty, *levelBeforeEscalation* will be the access level the GUI client was at when it escalated (provided by the own GUI Client), and *loggedUserId* will be the user that was logged at the time of the escalation (not necessarily the same user for whom the escalation happened).
* For failed deescalations, *success* will be *false* and *reason* will be a message describing the reason for the failure.

[2.16.0]_ *loginInformation*
userId (string)
accessLevel (int) [sent by non read-only GUI servers]

* Message sent upon successful validation of a one-time token sent by a GUI client as part of the authenticated login process.
* As its name states, a one-time token can only be validated once. Any further attempt to validate an already validated one-time token will result in a validation error.
* Upon receival of an invalid one-time token, the GUI Server immediately sends an error notification message to the client and closes the connection.
* The value of the ``accessLevel`` key is one of the values of the ``karabo::util::Schema::AccessLevel``
enumeration.

[repl_2.8.0]_ *configurationChanged (deviceConfiguration)*
deviceId (string)
configuration (Hash)

[2.8.0]_ *deviceConfigurations*
deviceConfigurations (Hash)

Hash contains all the configuration changes that happened to the monitored devices of interest to the Gui-client since the last
occurrence of a ``deviceConfigurations`` message.

An example hash of the ``deviceConfigurations`` message is shown below.

::

     'type' => deviceConfigurations STRING
     'configurations' +
       'cppServer/1_PropertyTest' +
         'outputCounter' => 32 INT32
       'cppServer/2_PropertyTest' +
         'outputCounter' => 48 INT32

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
           'DataLogger-clog_0' type="device" classId="DataLogger" serverId="karabo/dataLogger" visibility="4" compatibility="1.0" host="exflqr30450" status="ok" archive="0" capabilities="0" heartbeatInterval="60" KaraboVersion="3913949" +
           'DataLogger-Karabo_AlarmService' type="device" classId="DataLogger" serverId="karabo/dataLogger" visibility="4" compatibility="1.0" host="exflqr30450" status="ok" archive="0" capabilities="0" heartbeatInterval="60" karaboVersion="3913949" +
       'update' +
         'device' +
           'cppServer/1_PropertyTest' type="device" classId="PropertyTest" serverId="cppServer/1" visibility="4" compatibility="1.0" host="exflqr30450" status="ok" archive="1" capabilities="0" heartbeatInterval="120" karaboVersion="3913949" +
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

[2.20.0]_ *escalate*
escalationToken (string)
version (string) (GUI Client version)
levelBeforeEscalation (int - matching a karabo::util::Schema::AccessLevel value)

* Message a GUI client sends to request an Access Level escalation to the GUI Server. The GUI Client sends this message after it has already authenticated the user requesting the escalation.
* The *escalationToken* is a one-time token that the GUI Server will validate and authorize with the help of the Karabo Auth Server.
* The *levelBeforeEscalation* is the access level the escalating GUI Client had at the time it requested the escalation. It will be sent back by the GUI Server when the deescalation happens (either by expiration or upon a request from the GUI client).
* The GUI Server will send an *onEscalate* message later to the requesting GUI client with the results for the escalation request.

[2.20.0]_ *deescalate*
escalationToken (string)
version (string) (GUI Client version)

* Message a GUI client sends to request an Access Level deescalation to the GUI Server.
* The *escalationToken* must match the token sent with the corresponding *escalate* request.
* The GUI Server will send an *onDeescalate* message later to the requesting GUI client with the results of the deescalation request.

*login*
[depr_2.16.0]_ username (string)
[2.16.0]_ clientId (string) (GUI Client Hostname and PID)
[2.16.0]_ oneTimeToken (string) (Sent for authenticated logins)
version (string) (GUI Client version)
[2.16.0]_ clientUserId (string) (Sent for non-authenticated logins)

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
.. [repl_2.8.0] Last used in Karabo 2.7.X - replaced by *deviceConfigurations* (note below).
.. [2.8.0] Introduced in Karabo 2.8.0 to enable bulk updates of device configurations in the client.
.. [2.16.0] Introduced in Karabo 2.16.0 to support User authentication.
.. [depr_2.16.0] Deprecated in Karabo 2.16.0: "username" transporting the "clientId" of the GUI Client instance deprecated.  "clientId" and "clientUserId" used to send the id of the GUI Client (host and PID) and the Id of the user running the GUI Client (for non-authenticated logins). Access Level only transmitted from the server to the client as a result of token validation; otherwise the GUI Client adopts the access level selected by the user at login time.
.. [2.20.0] Introduced in Karabo 2.20.0 to support temporary Access Level escalations for User Authenticated sessions.