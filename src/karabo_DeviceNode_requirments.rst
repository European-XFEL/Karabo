*****************************************************
Requirements for control of sub-devices in MDL device
*****************************************************

Situation:
==========
One middle-layer device that controls, monitors etc. one or several other devices of any type (called sub-devices).

Connectivity:
=============
* Connect to all sub-devices. Give feedback if any and which could not be reached in a way that the MDL device can take action, like changing state and displaying this info in the status field (not possible with DeviceNode).
* Monitor all sub-devices constantly and notify if a connection is lost. Action to be taken could be just a state change of the MDL device and a status message, or trying to re-connect automatically first.

Access to sub-devices:
======================
* execute slots
* set and read all attributes from schema of sub-device, also those injected at a possibly later stage after initial connection had been established (not possible with DeviceNode)


Design Question:
================
Connectivity:
-------------
This can in principle be realized by using connectDevice (used in LPDComposite for example to get the feedback which sub-device did not connect). In principle every MDL device with sub-devices should have the above requested capability. Therefor the following design questions come to mind:
* How much code is needed to satisfy above requirements?
* Can it be generic, in the sense that is could be extracted into a package that can be used and possibly configured by any MDL device to avoid duplicating this code over and over again? Maybe something like a service where I can register all sub-devices which then takes care of connection and monitoring?

Monitoring sub-devices
----------------------
Using connectDevice or getDevice (both give a karabo.middlelayer_api.proxy) in ikarabo: When the sub-device is gone, attributes are still returned containing the last value. The state of sub-device does change to UNKNOWN, but one can't distinguish if in UNKNOWN because of connection lost or due to other operation problem.
In principle is Alive needs to be True whenever information from the sub-device is used. Needs to monitored constantly, e.g in background function and act on connection lost. Should be standard component of MDL device to monitor sub-devices.

Access to attributes in Nodes
-----------------------------
Unrelated to sub-device control and monitoring, is the question of getting attributes from the schema that are nested in nodes and the key-name is a parameter, so not fixed. Acess in fixed case is easy via
val = mydevice.node.subnode.mykey
But if the key is for example configurable like in genericValidation the MDL uses getattr from basic python functionality to get those from the sub-device. This can not take a key that is nested, e.g
getattr(mydevice,"node.subnode.mykey"). Each device-code would have to recurse through the node (like we implemented in genericValidation). It would be great to have that as a generic convenience function in the MDL api. (In the bound API one can use the get() function of the base device (not proxy) with a nested key name.)
