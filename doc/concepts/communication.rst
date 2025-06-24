..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

*****************************
Communication
*****************************

.. _broker:

Broker-based Communication
==============================

An important aspect in Karabo's distributed communication concepts are
distributed message brokers. The broker(s) relay both housekeeping information,
e.g. heartbeats of devices and slow control parameters. Devices connect
themselves to a topic on the broker.



Data Pipelines
==============================

In cases where large amounts of data, e.g. images, digitizer data or
any pulse resolved data is to be passed
between devices or to the DAQ system, data pipeline channels are to be used.
These are point-to-point connections using TCP/IP. The connection details
like host and port are exchanged using broker communication.

Each device can have several output and input channels. They are identified
within Karabo by their channel id which is composed of the device id, a colon
':' and a device unique channel name, e.g. "A/DEVICE/WITH_OUTPUT:output_1".
Each input channel has to be configured to which output channels (identified
by their output channel ids) it should be connected. Karabo takes care that
the connection between an input and an output channel is established whenever
their devices join the Karabo system.

The data is sent as a Karabo Hash, accompanied with meta data.
This meta data contains a common timestamp for all the data in the Hash and
the data source, usually the id of the output channel.
Whenever data for an input channel is available, a previously registered
**data** handler is called with the data and the meta data as arguments.

Alternatively, an **input** handler can be registered. It will be called with
the input channel as argument and the handler has to loop over potentially
several data items. The number of data items available in one go in this case
depends on the input channel configuration and the way that the output channel
sends them.
The main use case for that are calibration or analysis
devices that receive live data from the DAQ.

If a logical end of a series of data items is reached, the sending device can
trigger its output channel to send an end-of-stream notification. To get
notified about that situation, one can register and **end-of-stream** handler
at the input channel. It will be called with the input channel as an argument.
For example, a camera device sending acquired data via an output channel should
notify end-of-stream if it stops the acquisition. A connected image processing
device could then mark itself as not processing anymore.

Note that in C++ and bound Python, the device methods *writeChannel* and
*signalEndOfStream* (and the underlying output channel methods *write*,
*update*, *signalEndOfStream*) are not thread safe in the sense that they must
not be called concurrently with each other for the same output channel.

Many input channels can be connected to the same output channel (scatter
scenario) and many output channels can send data to the same input channel
(gather scenario).

Channel Configurations
++++++++++++++++++++++++++++++++++++++

Input and/or output channels of a device are usually described in the schema
of their device (using *expectedParameters* function in the C++ and bound Python
APIs). Their behaviour is then steered by the configuration properties as
explained in the following. (To be checked whether these match those in the
middlelayer API.) The properties can be configured together with any other
properties of the respective devices.

It is also possible to create input and output channels on-the-fly, but that
is not recommended. If an input channel is needed to connect to output channels
that cannot yet be defined before the device is instantiated, it is recommended
in the C++ and bound Python APIs to use the *registerChannelMonitor* method of
the device client available via *remote()*. For an example see the
*GuiServerDevice* of the framework.


Input Channel Configuration Properties
----------------------------------------

* **connectedOutputChannels** (VECTOR_STRING): One or several output channels from which data should be received, e.g. "A/DEVICE/WITH_OUTPUT:output_1".
* **dataDistribution** (STRING): Whether the data received from the output channel(s) should be shared with other input channels or not.
   * *shared*: Each data item is sent only to one of all the input channels connected in this data distribution mode.
   * *copy* (default): This input channel will receive data irrespective of other input channels connected to the same output channel.
* **onSlowness** (STRING): Policy for what the output channel should do if this input is too slow for the fed data rate - only used in *copy* mode (whereas in *shared* mode the output channel configuration defines the behaviour, see below).
   * *drop*: The output channel drops data items.
   * *wait*: The output channel writing action blocks until the input channel is ready for new data.
   * *queueDrop*: The data will be queued in the output channel and sent as soon as the input channel is ready. If the queue is full (size about 2000 in C++ and bound Python)Like *queue*, but if thet queue is full, its oldest data is dropped
   * *queue*: Removed option since 3.0.0, same as *queueDrop* before (since 2.10.0).
   * The default is *drop*.
* **minData** (UINT32): Minimum data items available in one go if an input handler is used instead of a data handler (default = 1, 0: all, 0xFFFFFFFF: none/any).
* **respondToEndOfStream** (BOOL): Whether an end-of-stream handler that a device registered is called or not (default: true).
* **delayOnInput** (INT32): Delay in milliseconds before informing output channel about readiness for next data (default: 0). Useful e.g. with onSlowness=*drop* if only a limited data rate is of interest.

Output Channel Configuration Properties
----------------------------------------

* **distributionMode** (STRING): Ignored since 2.19.0, removed since 3.0.0.
* **noInputShared** (STRING): If input channels are connected in *shared* data distribution mode, defines what to do if none of them is ready to receive, but data shall be sent.
   * options are *drop* (default since 2.10.0), *queue*, *wait* (default till 2.9.X) and since 2.10.0 *queueDrop*.  Option *throw* exists until 2.9.X but is not recommended. See description  of the **onSlowness** property of input channels.
* **hostname** (STRING): Hostname or IP address to which input channels shall connect. Default is "default" which means the hostname of the device. Otherwise one can specify the address of a second network card connected to e.g. a special high band width network.
* **port** (UINT32): Port number which input channels shall address when establishing their TCP connections to the output channel. Default is 0 which means that the system will provide a port number. If another value is chosen, the port must be free and accessible.

Distribution between several connected *shared* input channels is usually *load-balanced*, i.e. any input channel that is ready for more data can receive it.
This behaviour is overwritten if the device code registers a *SharedInputSelector* at the output channel.

Schema Description of Channels
--------------------------------------
While the underlying protocol is able to send and receive hashes of any
structure that may even vary from data item to data item, it is strongly
recommended that output channels define the structure of the data they sent and
that input channels define which data structure they expect.
For output channels this is required if the data shall be stored by the DAQ or
visualised in the Karabo GUI.

In C++ and bound Python, the data schema can be specified when the channels are
defined in the *expectedParameters* function of the device.
