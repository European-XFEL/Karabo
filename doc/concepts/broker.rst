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

Shared Resources
================

A shared resource is a device that is configured to communicate with more than
one topic. Examples are e.g. the tunnel installations of a given SASE which is
shared by both instruments in the SASE or the shared laser installations.

Devices can be configured to not interpret assignment and command messages from
a topic they are configured to communicated with. This allows for exclusive
resource sharing. For example, one of two experiments sharing a laser
installation can request exclusive rights to the shared laser devices by
requesting to not interpret command and assignment messages from the topic the
other instrument is configured to. Say the first experiment uses topic
*exp1* and the second topic *exp2*. If *exp1* needs exclusive access, it would
configure all laser devices to ignore messages from topic *exp2*. The experiment
on *exp2* would still be able to view the current values on all devices though,
as property update messages would still be sent to both topics.

.. note::

    A final remaining topic must always be enabled for assignment message and command
    message evaluation, as otherwise the device would not be configurable
    anymore. Specifically a message blocking reconfiguration to a single remaining
    topic will not be applied by the device.

.. graphviz::
    :caption: Example of shared resources, with devices attached to topics *exp1* and *exp2* The green device, *EXP2/DEVC* is a shared resource and visible for the client attached to topic *exp1*.


    digraph resource_sharing{

    exp1
    [
      shape = octagon
      label = "broker:exp1"
    ]

    exp2
    [
      shape = octagon
      label = "broker:exp2"
    ]

    exp1devA
    [
      shape = box
      label = "EXP1/DEVA"
    ]

    exp1devB
    [
      shape = box
      label = "EXP1/DEVB"
    ]

    exp2devA
    [
      shape = box
      label = "EXP2/DEVA"
    ]

    exp2devC
    [
      shape = box
      style = filled
      fillcolor = chartreuse1
      label = "EXP2/DEVC"
    ]

    clientExp1
    [
      shape = none
      label = <<table><tr><td>client:exp1</td></tr>
                      <tr><td>EXP1/DEVA</td></tr>
                      <tr><td>EXP1/DEVB</td></tr>
                      <tr><td>EXP2/DEVC</td></tr>
              </table>>
    ]

    exp1devA -> exp1[dir = both]
    exp1devB -> exp1[dir = both]
    exp2devA -> exp2[dir = both]
    exp2devC -> exp2[dir = both]
    exp2devC -> exp1[dir = both]
    exp1 -> clientExp1[dir = both]

    }

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


