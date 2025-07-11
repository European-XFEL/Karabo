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

*serverInformation*
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

[2.20.0]_ *onTemporarySessionExpired*
expiredToken (string),
expirationTime (string),
levelBeforeTemporarySession (int - matching a karabo::util::Schema::AccessLevel value),
loggedUserId (string)

* Message sent by the GUI Server to the client whose temporary session has expired. The GUI Server checks for expired temporary sessions every 10 seconds; most likely there will be a delay between the expiration and the client being communicated about it.
* The *expirationTime* string is the UTC date and time of the expiration in ISO-8601 format, e.g, "20240205T120633.139529Z".

[2.20.0]_ *onEndTemporarySessionNotice*
aboutToExpireToken (string),
secondsToExpiration (unsigned int 64)

* Message sent by the GUI Server to the client whose temporary session is about to expire. The GUI Server checks for temporary sessions lifetimes every 10 seconds.
* A temporary session is considered "about to expire" if its expiration time is less than "endTemporarySessionNoticeTime" seconds away. "endTemporarySessionNoticeTime" is an init-only property of the GUI Server.

[2.20.0]_ *onBeginTemporarySession*
success (bool),
reason (string),
accessLevel (int - matching a karabo::util::Schema::AccessLevel value),
temporarySessionDurationSecs (int)

* Message sent by the GUI Server with the results of beginning a temporary session to the requesting client.
* For successful temporary session beginings, *success* will be *true*, *reason* will be empty, *accessLevel* will be the access level of the temporary session, and *temporarySessionDurationSecs* will be the lifetime of the temporary session in seconds.
* For rejected temporary sessions beginings, *success* will be *false* and *reason* will be a message describing the reason for the rejection.

[2.20.0]_ *onEndTemporarySession*
success (bool),
reason (string),
levelBeforeTemporarySession (int - matching a karabo::util::Schema::AccessLevel value),
loggedUserId (string)

* Message sent by the GUI Server with the results of ending a temporary session to the resquesting client.
* For successful endings, *success* will be *true*, *reason* will be empty, *levelBeforeTemporarySession* will be the access level the GUI client was at when the temporary session began (provided by the own GUI Client), and *loggedUserId* will be the user that was logged at the time before the start of the temporary session (nearly always a different user).
* For failed endings, *success* will be *false* and *reason* will be a message describing the reason for the failure.

[2.16.0]_ *loginInformation*
userId (string)
accessLevel (int) [sent by non read-only GUI servers]
readOnly [2.20.0]_ (bool) from 2.20.0 this is ``true`` for login requests with no one-time token.

* Message sent upon successful validation of a one-time token sent by a GUI client as part of the authenticated login process.
* As its name states, a one-time token can only be validated once. Any further attempt to validate an already validated one-time token will result in a validation error.
* Upon receival of an invalid one-time token, the GUI Server immediately sends an error notification message to the client and closes the connection.
* The value of the ``accessLevel`` key is one of the values of the ``karabo::util::Schema::AccessLevel``
enumeration.
* Since 2.20.0 a ``login`` message can be sent to an authenticated GUI Server with no one-time token. This is interpreted as a request to start a read-only session for the requesting client. The authorization is bypassed and the user is limited to the ``OBSERVER`` access level.

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

[2.20.0]_ *beginTemporarySession*
temporarySessionToken (string)
version (string) (GUI Client version)
levelBeforeTemporarySession (int - matching a karabo::util::Schema::AccessLevel value)

* Message a GUI client sends to request a temporary session the GUI Server. The GUI Client sends this message after it has already authenticated the user requesting the temporary session.
* The *temporarySessionToken* is a one-time token that the GUI Server will validate and authorize with the help of the Karabo Auth Server.
* The *levelBeforeTemporarySession* is the access level the GUI Client had at the time it requested the begining of the temporary session. It will be sent back by the GUI Server when the temporary session ends (either by expiration or upon a request from the GUI client).
* The GUI Server will send an *onBeginTemporarySession* message later to the requesting GUI client with the results for the begin temporary session request.

[2.20.0]_ *endTemporarySession*
temporarySessionToken (string)
version (string) (GUI Client version)

* Message a GUI client sends to request the GUI Server to end a temporary session.
* The *temporarySessionToken* must match the token sent with the corresponding *beginTemporarySession* request.
* The GUI Server will send an *onEndTemporarySession* message later to the requesting GUI client with the results of the end temporary session request.

[3.0.0]_ *getGuiSessionInfo*

* Message a GUI client sends to request timing information about its sessions: the login session and an optional
  temporary session that might have been started on top of the login session.
* The response is returned immediately in the form of a Hash with the following keys and values:

type (string) - with the value "getGuiSessionInfo"
success (boolean) - true if the request was successful, false otherwise
reason (string) - error message for failed requests; empty for successful requests
sessionStartTime (string) - the UTC timestamp for the session start time in ISO 8601 format;
sessionDuration (unsigned int) - the maximum duration of a login session in seconds;
tempSessionStartTime (string)-  the UTC timestamp for the temporary session start time in ISO 8601 format (empty if there's no active temporary session);
tempSessionDuration (unsigned int) - the maximum duration of a temporary session in seconds.

*login*
[depr_2.16.0]_ username (string)
[2.16.0]_ clientId (string) (GUI Client Hostname and PID)
[2.16.0]_ oneTimeToken (string) (Sent for authenticated logins; can be omitted for authenticated logins since 2.20.0; more details on description of ``loginInformation`` above)
version (string) (GUI Client version)
[2.16.0]_ clientUserId (string) (Sent for non-authenticated logins)
[2.20.0]_ application (boolean) (False for the standard GUI Client; True for its variants like Karabo Cinema and Karabo Theater)

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
.. [2.20.0] Introduced in Karabo 2.20.0 to support temporary sessions on top of User Authenticated sessions.
.. [3.0.0] Introduced in Karabo 3.0.0 to provide session duration information to a connected GUI Client. 3.0.0 enforces a maximum duration to the login session.