.. _broker:

*****************************
The Central Message Broker
*****************************

An important aspect in Karabo's distributed communication concepts are
distributed message brokers. The broker(s) relay both housekeeping information,
e.g. heartbeats of devices and slow control parameters. Devices connect
themselves to a topic on the broker.

A device may be configured to listen to and send messages to any number of
broker topics through an (expert-only) expected parameter.
Specifically, a device configured to use more than one topic will
react on incoming messages from any configured topic and will send a copy of
every outgoing message to all configured topics. On the broker side, message
copies which have no consumer in a given topic are simply discarded.

Non-Broker-based Communication
==============================

In addition to broker-based communication Karabo uses direct point-to-point
communications in two scenarios. Here the broker merely acts as a name server
to initiate the connections, but afterwards data flows bypass the broker.

Data Loggers
++++++++++++

Connect to the slow control properties of a device via a point-to-point
connection because they devour a large amount of data. In this way logging
messages does not stress the broker unnecessarily. See Section :ref:`data_logging`
for details.

P2P Communication For Pipelined Processing and DAQ
++++++++++++++++++++++++++++++++++++++++++++++++++

In cases where large amounts of data, e.g. pulse resolved data is to be passed
between devices or to the DAQ system, p2p channels supporting e.g. scatter/
gather topologies are to be used. Data passed and received via these channels
also bypasses the broker and allows for easy aggregation of complex data
hierarchies into composite data messages.


