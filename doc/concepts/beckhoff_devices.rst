****************
Beckhoff Devices
****************

The BeckhoffCom Device
======================

Karabo interacts with the Beckhoff PLC system via a generic communication device, the
*BeckhoffCom* Device. This device is responsible for message passing between Karabo and
the PLC system, managing connections to the PLC, as well as exposing the properties and
commands available on the PLC side to the distributed control system.

Using the BeckhoffCom Device
++++++++++++++++++++++++++++

Prior to initialization the *BeckhoffCom* Device needs to be configured to the PLC it is
to communicate with. This is done using the following expected parameters:

PLC server
   is a unique resource identifier (URI) for the PLC to communicate with. It is given in
   the form *tcp://host:port*. *tcp://* can be omitted, since it is the default. If no
   *port* is specified, the default port 1234 is used.
   
Server Timeout
   is the maximum timeout in milliseconds allowed when communicating with the PLC. If upon
   communication with the PLC no acknowledgement is received after this duration the 
   BeckhofCom device assumes a communication error and transitions to the *error* state.
   It defaults to 1000 ms, which should be sufficient even in congested network scenarios.
   
Autoreset Time
   is the time in seconds after which the BeckhoffCom device will attempt reinitiating
   communication with the PLC from an error state. Set it to *0* to disable this feature.
   By default it is set to *10s*. By allowing auto-reset Karabo can attempt to recover
   from a communication loss with the PLC.
   
Create Generic Devices
   will instruct the BeckhoffCom device to automatically create generic devices from the
   self-description of the PLC if no specific device for a PLC device instance is found.
   See Section `Self Description`_ for details.
   

Upon initialization the *BeckhoffCom* device automatically transitions into the *connecting*
state, which either, upon successful connection to the PLC, transitions to the *connected*
state, or in case of an error transitions to an *error* state.

The error state is also reached upon any communication errors in the *connected* state. The
user may then attempt to reste the device, using the *reset* slot. Executing a reset will
bring the device back into the *connecting* state, and a new connection attempt is 
initiated.

The following read-only properties convey information on the PLC after initialization:

PLC Uptime
    gives the total uptime of the PLC in seconds.
    
Available PLC devices
    list the available device instance on the PLC, which are **not** disabled. Details
    on disabled devices are given in Section `Disabling Devices on the PLC Side`_.
    
.. note::

	The BeckhoffCom device should usually be configured to archive to the data loggers.
	In this way connection errors and reset attempts are recorded, and may provide a
	valuable source of debugging information.


The Communication Model
+++++++++++++++++++++++

The communication model of the Beckhoff interface is described in [BeckhoffComm]_. Here
a short overview will be given.

As underlying network protocol TCP/IP is used, i.e. network error handling is already
taken care of on the protocol and packet layer. Messages are split into 32bit words,
in accordance with the TCP specification. Each message consists of a *header* and a 
*body*, the latter arranged into so-called *pairs*. The formats are given in the 
following.

.. note::

	Usually only PLC devices owning a message will interpret it. There are a few
	exceptions, notably, the device manager, which will take care of re-enabling
	previously disabled devices, as these would otherwise never interpret an
	enable request.


.. table:: Header format used for Karabo - Beckhoff communication.

	+--------------+--------------+--------------+--------------+
	|0-7           |8-15          |16-23         |24-31         |
	+--------------+--------------+--------------+--------------+
	|Message length                                             |
	+-----------------------------------------------------------+
	|Time epoch                                                 |
	+-----------------------------------------------------------+
	|Time frac.                                                 |
	+-----------------------------------------------------------+
	|Train ID low word                                          |
	+-----------------------------------------------------------+
	|Train ID high word                                         |
	+-----------------------------------------------------------+
	|Version ID                                                 |
	+-----------------------------------------------------------+
	|Number of pairs in body                                    |
	+-----------------------------------------------------------+
	|... Body: Pair 1 ...                                       |
	+-----------------------------------------------------------+
	|... Body: Pair 2 ...                                       |
	+-----------------------------------------------------------+
	|....                                                       |
	+-----------------------------------------------------------+
	|... Body: Pair n ...                                       |
	+-----------------------------------------------------------+
	
As is shown in the table, the header contains information on the message to follow,
including the number of value pairs to be expected in the body. Additionally, information
on the time of messaging is provided. The train-id given is the most current one for when
the message is sent and is injected into the PLC system from the XFEL timing system.
	
.. table:: Body pair format used for Karabo - Beckhoff communication.

	+--------------+--------------+--------------+--------------+
	|0-7           |8-15          |16-23         |24-31         |
	+--------------+--------------+--------------+--------------+
	|Class         |Coupler no.   |Softdevice no.|Channel no.   |
	+--------------+--------------+--------------+--------------+
	|Key name                                                   |
	+-----------------------------------------------------------+
	|Time frac.                                                 |
	+-----------------------------------------------------------+
	|Number of values                                           |
	+-----------------------------------------------------------+
	|Value 1                                                    |
	+-----------------------------------------------------------+
	|Value 2                                                    |
	+-----------------------------------------------------------+
	|....                                                       |
	+-----------------------------------------------------------+
	|Value n                                                    |
	+-----------------------------------------------------------+

The body of the message consists of a pair, containing both meta-information on the 
(list of) values sent, and the values itself. The meta-information contains information
on the device id in the first 32-bit word. The device id needs to uniquely identify all
devices in the PLC loop. It is classified by

Class
   Specifies the class of the device. An up-to-date table of device classes can be found
   `here <https://docs.xfel.eu/share/page/site/daqControls/wiki-page?title=Status_overview>`_.
   
Coupler no.
   Number of the EtherCAT coupler which the terminals/related hardware is connected to.
   One Device should always be connected to terminals on the same EtherCAT coupler.
   The DNS-Name of the PLC plus this number could provide information about the location
   of a Device.
   
Softdevice no.
   Individual number of the softdevice. Each class coupler combination starting with
   device number.
   
Channel
   Channel of the Softdevice. Some devices may have a number of channels sharing the
   same device instance. This channel is part of the softdevice and need not be identical
   to the hardware-channel of the terminal.
   
The next word contains the *key name*, which specifies what type of data is being sent:
it can be either a command, which is without values, or a property assignment/request.
The structure is as follows:

..  table:: Key word format used for Karabo - Beckhoff communication.

	+-----------------------------------------------+--+--+--+--+
	|0-27                                           |28|29|30|31|
	+-----------------------------------------------+--+--+--+--+
	|command/property identifier                    |R |EF|WF|CF|
	+-----------------------------------------------+--+--+--+--+
	
	
| R:  reserved
| EF: NACK/Error flag, 1 indicates an error
| WF: write flag, 1 indicates a write property message, 0 indicates read property
| CF: command flag, 1 indicates a command message

	
The following key names are reserved:

.. table:: Reserved key names in Karabo - Beckhoff communication.

    ================ =====================
    0x08000001       List devices
    0x08000002       Send greeting string
    0x08000003       Send heartbeat
    0x1000 - 0x1010  Send self-description
    ================ =====================

Finally, for each *pair* a *time fraction*, denoting the number of 100ns steps passed
with respect to the timing of the last train, and the number of values in the pair is
sent.

*Values* correspond to the actual payload data, arranged into 4 byte wide fields. This
does not mean that the data type of the value has to be 32 bits wide. Information on 
how to interpret the payload, i.e. type information for each *key name* is passed as part
of the Beckhoff self-description. Currently the following data-types are supported.

..  table:: Data types supported in Karabo - Beckhoff communication.
    
	=============== ============================================
	*Beckhoff Type* *Karabo Type*
	=============== ============================================
	tBOOL           BOOL
	tBYTE           UINT8
	tSINT           INT8
	tWORD           UINT16
	tINT            INT16
	tDWORD          UINT32
	tDINT           INT32
	tREAL           FLOAT
	tSTRING         STRING (max. 28 chars)
	tLREAL          DOUBLE
	tLINT           INT64
	tULINT          UINT64
	=============== ============================================


.. [BeckhoffComm] T. Freyermuth and J. Tolkiehn, European XFEL
    PLC Karabo Interface Description, 2015, XFEL.EU IN-2015-09-30

Data flow
+++++++++

Communication with a Beckhoff PLC is asymmetric: the Karabo control system sends messages
without timing information, but receives messages with timing information from the PLC.

From the control system to the PLC
    messages are acknowledged upon reception and then passed to an input buffer of 
    limited size. Each device on the PLC searches the buffer for message 
    with its device id, popping them from the buffer and then processing them. 
    
From the PLC to the control system
    messages are placed into an output buffer by the PLC's sending devices using provided
    functions. These function acknowledge the storage into the buffer. The buffer is
    sent out en-block when a treshold of contained messages is exceeded.
    
.. warning::

	Because of the input buffer's limited size, Karabo devices must take care to not 
	query the PLC system at unreasonably high rates, i.e. much faster than the 
	train-repetition frequency of 10 Hz.
	
Self Description
++++++++++++++++

Upon connection of the control system to a Beckhoff PLC, the PLC confirms it presence and
availability by sending a greeting message (device id 0x0C000101, key 0x08000002). It
then proceeds by sending a self-description of the *device classes* and *device instances*
it hosts. The self-description uses a set of reserved key names:

.. table:: Reserved key names in Karabo - Beckhoff Self Description.

    ================ ===========================================
    0x1000           Start of class description
    0x1001           Name of the property or command
    0x1002           Keyname of the property or command, 
    0x1003           Displayed name of the property or command
    0x1004           Description of the property or command
    0x1005           Start of instance description
    0x1010           End of selfdescription (class or instance) 
    ================ ===========================================
    
As is evident from the table the fields correspond to a subset of attributes available
for Karabo's expected parameters. The *beckhoffCom* interprets this self-description
and creates a Karabo device schema there-from. 

Additionally, the option of autogenerating a Karabo-device from the self-description is
provided. If the corresponding flag (*createGenericDevices*) is checked on the
*BeckhoffCom* device, Karabo device instances for all Beckhoff devices which do not have
a corresponding Beckhoff device instantiated are created.

Disabling Devices on the PLC Side
+++++++++++++++++++++++++++++++++

Especially for testing, or when a (non-essential) device in a complex Beckhoff-loop is
'misbehaving' it might be reasonable to interface only a subset of the devices with the
PLC. This can be done by *disabling* the softdevices on the PLC side. Disabled devices
will not participate in message passing any more and will not make their presence aware
through self description. Enabling or disabling of a device can be done using the 
corresponding slots (administrator privileges needed), on the Karabo instance of the
device.

Interaction with the PLC via BeckhoffCom
++++++++++++++++++++++++++++++++++++++++

After having initiated communication with a PLC, with associated reception of the PLC's
self-description the BeckhoffCom device mediates message passing from and to Karabo
for devices wanting to interface with the PLC. For this a number of *slots* are available
on the BeckhoffCom device, which can be called using the request-reply pattern. These slots
are:

.. function:: reset
   
   is used to reset the BeckhoffCom device and reinitiate communication with the PLC
   
.. function::  plcReadRequest(unsigned int requestId, string deviceName, Hash propertyList)
   
   is used to request a property from the PLC device identified by device name. The request
   id should be a unique id to identify requests from the same Karabo device. The
   propertyList hash encodes the properties to be requested in the form 
   *h[propertyName] = 0u*. After completion of the read request on the PLC side, a reply is
   sent back to the requester containing the request id and the read value and status code
   of the read operation. The status code is a number, attached as an attribute named
   *status* to each read value.
   
.. function:: plcWriteRequest(unsigned int requestId, string deviceName, Hash propertyList)
   
   is used to write a property to the PLC device identified by device name. The request
   id should be a unique id to identify requests from the same Karabo device. The
   propertyList hash encodes the properties to be written in the form 
   *h[propertyName] = value*. After completion of the write request on the PLC side, a reply is
   sent back to the requester containing the request id and the status code
   of the write operation. The status code is a number, attached as an attribute named
   *status* to each write value.
   
.. function:: plcCallRequest(unsigned int requestId, string deviceName, Hash callValue)
   
   is used to execute an action on PLC device identified by device name. The request
   id should be a unique id identify requests from the same Karabo device. The
   callValue hash encodes the command to be executed in the form
   *h[commandName] = 0*. After completion of the call request on the PLC side,  a reply is
   sent back to the requester containing the request id and the status code
   of the call operation. The status code is a number, attached as an attribute named
   *status* to each call value.
   
.. function:: plcBrowseRequest(string root)
   
   is used to request the schema of the PLC device identified by the name *root*. It will
   be returned as part of a request/reply pattern.
   
.. warning::

	You should normally not have to implement the read, write and call requests patterns
	in your own device, but rather inherit from the *BeckhoffDevice* base class, which
	already implements these. See this `section <beckhoffDeviceBase>`_ for details.

      

.. beckhoffDeviceBase_

The BeckhoffDevice Base Class
=============================

Most frequently, users will not have to care with the low-level details of Karabo-Beckhoff
communication via the *BeckhoffCom* device as introduced in Section 
`The BeckhoffCom Device`_. Instead specific Beckhoff devices should inherit from the
*BeckhoffDevice* base class, which already implements the communication patterns and
exposes them through slots and allows customization using predefined properties.

BeckhoffCom Device
   is the instance id of the *beckhoffCom* Device which will mediate communication with
   the PLC. See Section `The BeckhoffCom Device`_ for details.
   
.. note::

	The BeckhoffCom device will register itself automatically with each beckhoff device
	that it finds in the self description from the PLC. Therefore, this field is not
	changeable by users.
	
	
Properties to poll
   is a list of properties to be polled from the PLC device at the given polling interval.
   This property is reconfigurable after initialization.
   
Poll interval
   give the polling interval in seconds for properties specified in the
   *Properties to poll* field. It defaults to 30 s. This property is reconfigurable
   after initialization.
   
.. warning::

   Communication with the Beckhoff PLC is by default event driven. Polling should thus
   be used only for usage scenarios mandating it.
   
.. warning::

   Do not configure the polling interval to very small values as this will lead to
   excessive network traffic and requests on the PLC, the ladder bearing the danger
   of buffer overflows of the receive buffer. If you are in doubt and need a polling
   interval of less than a second contact a user support person first.

Properties to read
   is a list of properties to be read from the PLC device if the read command is executed.
   This property is reconfigurable after initialization.
   
.. note::
   
   Communication with the Beckhoff PLC is by default event driven. The read command or 
   polling should only be used if you need to make sure that you have the most 
   up-to-date value from the hardware at a very specific point in time,
   as mandated by some external requirement.
   
   
The Statemachine of the BeckhoffDevice
++++++++++++++++++++++++++++++++++++++

.. digraph:: state_transitions

   "INIT"->"OFF"[style="dashed"]
   "INIT"->"ON"[style="dashed"]
   "INIT"->"CHANGING"[style="dashed"]
   "OFF"->"ON"[label="on"]
   "ON" -> "CHANGING"[label="change"]
   "CHANGING" -> "ON"[label="abort"]
   "CHANGING" -> "ON"[label="on-target", style="dashed"]
   "CHANGING" -> "CHANGING"[label="change"]
   "ON" -> "RUNNING"[label="start", style="dashed"]
   "RUNNING" -> "ON"[label="stop", style="dashed"]
   "ON"->"OFF"[label="off"]
   "*"->"UNKNOWN"[label="software error", style="dotted"]
   "UNKNOWN" -> "INIT"[label="re-instantiate", color="red"]
   "*" -> "DISABLED"[label="interlock"]
   "*" -> "DISABLED"[label="manual"]
   "DISABLED"->"OFF"[style="dashed"]
   "DISABLED"->"ON"[style="dashed"]
   "DISABLED"->"CHANGING"[style="dashed"]
   "*" -> "ERROR"[label="hw error"]
   "ERROR"->"OFF"[style="dashed"]
   "ERROR"->"ON"[style="dashed"]
   "ERROR"->"CHANGING"[style="dashed"]
   
      

Implementing A Specialized Beckhoff Class
++++++++++++++++++++++++++++++++++++++++++

To understand how to implement a specialized Beckhoff class, which derives from the
*BeckhoffDevice* base class, it is important to first understand how the Karabo device
represents a view on the PLC device.

Whenever to the *BeckhoffCom* connects to a PLC it tries to match the the device
instance information it receives from the PLC to Karabo devices instances already 
present in the distributed system. If it cannot find any instance matching the instance
id forwarded by the PLC it optionally will instantiate a generic Karabo device for this
Beckhoff soft device.

After all Beckhoff devices have been matched with device instances in the distributed
system, the BeckhoffCom devices registers itself on these instances, and then initializes
their connection to itself by emitting a connection event.

Similarly, the BeckhoffCom deregisters itself on the instances connected to it, upon loss
of connection to the PLC. Before doing so it initiates transition to the disconnected
state on all devices by emitting a disconnect event. 

For developers of specific Beckhoff devices this means that they do *not* have to worry
about maintaining connections to the PLC, if the following preconditions are met:

- the device id of the specific instance matches a device id registered in the PLC
- the specific device is instantiated before the BeckhoffCom Device.

The latter condition is only relevant if BeckhoffCom is instructed to create generic
devices, which it would do if the specific device is not yet instantiated. If BeckhoffCom
is not instructed to create generic devices, the start sequence of BeckhoffCom
and specific Beckhoff devices is irrelevant, since BeckhoffCom registers itself
with all known Beckhoff devices regularly every 5 seconds.

.. warning::

	The creation of generic Karabo devices by the BeckhoffCom device should usually be
	restricted to testing purposes only. In production environments preference should be
	given to specific Karabo devices.
	
It is now up to the device programmer to make sure that the specific device class 
implemented matches the properties and commands exposed by the Beckhoff softdevice. 
We will demonstrate how this is done using the example of a digital output.

First we definine aliases for the expected properties of the specific Karabo
device class. These aliases need to match with the property names of the PLC soft devices
(see Section `The Communication Model`_). You do not need to generate the alias definition
on your own though. Rather a program called *propgen* will extract them from the
firmware definition. Such a file looks as follows.

.. code-block:: C++

	// Auto generated file! DO NOT EDIT!
	// To regenerate, run 'propgen SD_digitalOut'.

	#pragma once

	// SD_digitalOut properties
	#define  SD_DIGITALOUT_AINVERTVALUE	"AinvertValue"	// key=0x0000100
	#define  SD_DIGITALOUT_COFF	"COff"	// key=0x80000032
	#define  SD_DIGITALOUT_CON	"COn"	// key=0x80000031

.. note::

   Usually, the firmware developers from the Advanced Electronics group will provide
   an alias-to-key definition file for each Beckhoff device.
   
Using these definitions the expected properties in the Karabo device can be aliased:

.. code-block:: C++
   
   void BeckhoffDigitalOutput::expectedParameters(Schema& expected) {
        
        BOOL_ELEMENT(expected).key("invert")
                .alias<string>(SD_DIGITALOUT_AINVERTVALUE)
                .tags("plc")
                .description("Invert digital value.")
                .displayedName("Invert Value")
                .assignmentOptional().defaultValue(false)
                .expertAccess()
                .reconfigurable()
                .commit();

        SLOT_ELEMENT(expected).key("on")
                .alias<string>(SD_DIGITALOUT_CON)
                .displayedName("On")
                .description("Instructs the device to switch on")
                .allowedStates(STATE_OFF)
                .commit();

        SLOT_ELEMENT(expected).key("off")
                .alias<string>(SD_DIGITALOUT_COFF)
                .displayedName("Off")
                .description("Instructs device to switch off")
                .allowedStates(STATE_ON)
                .commit();
    }

We can then use the accustomed signal-slot technology to link the slot definition to 
command execution:

.. code-block:: C++

    BeckhoffDigitalOutput::BeckhoffDigitalOutput(const karabo::util::Hash& config)
                          : BeckhoffDevice(config) {
        SLOT0(on)
        SLOT0(off)
    }
    
    void BeckhoffDigitalOutput::on() {
        boost::mutex::scoped_lock lock(commandLock);
        plcCall(getAliasFromKey<string>("on"));
        waitForStateChangedTo(STATE_ON);
    }


    void BeckhoffDigitalOutput::off() {
        boost::mutex::scoped_lock lock(commandLock);
        plcCall(getAliasFromKey<string>("off"));
        waitForStateChangedTo(STATE_OFF);
    }

In the example code-block three methods of the BeckhoffDevice base class are introduced

.. function:: plcCall(const string& commandKey)
   
   will forward an execution request to the BeckhoffCom device managing communication with
   the PLC. It expects a command key, as defined by the Beckhoff soft device as argument.
   
.. function:: getAliasFromKey(const string& propertyName)
   
   can be used to retrieve a Beckhoff key, held in alias attribute of a Karabo property
   by the properties name.
   
.. function:: waitForStateChangedTo(const State& state)
   
   will block until the PLC device reaches the specified  state. This is possible
   because the PLC may trigger an update properties through an event driven fashion.
   
To complete our example we should make sure we update the state of the Karabo device
according to the PLC hardware state. This is done using the *decodeHardwareState* hook
of the base class.

.. code-block:: C++

	std::string BeckhoffDigitalOutput::decodeHardwareState(const std::bitset<32>& bits) {
        if (bits.test(12)) return STATE_ON;
        return STATE_OFF;
    }
    
In addition to the BeckhoffDevice methods introduced as part of the example,
the following slots are available:

.. function:: reset
   
   will trigger a reset of the *Karabo* device. The device will try to reinitiate
   connection to the hardware via a BeckhoffCom device and if successful transition into
   the *connected* state. Resets can be performed from the *error* state which denotes
   that a *software error in the Karabo device* has occurred.
   
.. function:: resetHardware
   
   will trigger a reset on the Beckhoff PLC soft device, i.e. the actual hardware. The
   device will transition to the *normal* state if the reset is successful. During the
   reset the connection to the PLC will be maintained. Resetting the hardware is 
   possible if the device is in a *HardwareError* state.
	
.. function:: safe
   
   transitions the PLC soft device into *safe* mode, which should be implemented on the
   firmware side so that the hardware protects itself and minimizes impact on its
   surroundings. Transitioning to safe mode is possible in the *normal* operating state.
	
.. function:: normal
   
   is the counterpart of safe mode and designates normal hardware operation will all
   supported and implemented functionality. The *normal* staste may directly only be
   entered with this command from *safe* mode, i.e the safe state. Returning to normal
   mode from the *HardwareError* state is done via the *resetHardware* command.
	
.. function:: requestHardwareValues
   
   initiates an update of the Karabo device properties to the current hardware values
   they map to.
	
.. function:: readHardware
   
   initiates an update of the Karabo device properties defined in the *Properties to read*
   property to the current hardware values they map to.
	
Devices deriving from *BeckhoffDevice* may also override the following user hooks,
implemented as protected virtual methods:

.. function:: onIntendedHardwareReconfiguration(Hash& hardwareData)
   
   allows to modify data to be sent to the PLC hardware as part of a device
   reconfiguration. The hash passed as a reference should be modified directly and 
   will contain the reconfigured parameters of the newly applied configuration.
	
.. function:: onPlcUpdate(const Hash& data)
   
   is executed after the device has received an update from the PLC hardware. It is
   called *after* the expected parameters of the device have been updated. It does
   thus not make a different if a property is accessed from the data Hash, which is 
   argument to the call, or by requesting a property via *get*. The latter of course
   involves slightly more overhead.
	
.. note::

	onPlcUpdate is the hook which should be used to react on event-driven updates from 
	the PLC.

	
Finally, the following methods are available for interacting with the PLC. The methods
are blocking until the PLC has responded:

.. function:: plcRead(const vector<string>& keys)
   
   reads hardware values of the key list passed as argument to the call. 
   keys can be either the key name of the exp. parameter or the alias.
   The call is mediated by the *BeckhoffCom* device.
   
.. function:: plcWrite(const Hash& values)
   
   writes the key/value pairs passed in the *values* Hash to the PLC.
   The keys need to match the property names defined in the PLC soft device
   (the alias) and the values need to be of the correct type.

.. function:: plcCall(const string&  commandKey)
   
   will forward an execution request to the BeckhoffCom device managing communication with
   the PLC. It expects a command name as defined by the Beckhoff soft device (the alias)
   as an argument.
   
.. warning::

	These calls should not be used for update the expected parameters of the Karabo
	device from the PLC (done in an event driven fashion) or the PLC after altering
	an expected parameter (handled by the base class).
	
   
.. function:: getAliasFromKey(const string& propertyName)
   
   can be used to retrieve a Beckhoff key, held in alias attribute of a Karabo property
   by the properties name.
	

.. function:: waitForStateUpdate
   
   blocks until the hardware state has changed.

.. function:: waitForStateChangedTo(const State& state)
   
   blocks until the PLC device reaches the specified state. This is possible
   because the PLC may trigger an update properties through an event driven fashion.   

.. todo::

	Implement unified states in the Beckhoff devices.
	
.. warning::
	
	Although both the BeckhoffDevice base class and the BeckhoffCom class provide some
	protection against malformed communication attempts to the PLC, developers should 
	carefully check the aliases they have defined, and may sure they are unique.

Methods in the Public Interface Used By BeckhoffCom
+++++++++++++++++++++++++++++++++++++++++++++++++++

The interface of the *BeckhoffDevice* class additionally contains slots which are used
by the *BeckhoffCom* device to interact with the devices accessing the hardware through
it. For completeness they are mentioned in the following, but developers generally need
not to worry about them when deriving from *BeckhoffDevice*. 

Two slots exist, so that a *BeckhoffCom* device can register and deregister
itself from *BeckhoffDevices*.

.. function:: registerComDevice(const string& comDeviceId)
   
   registers the *BeckhoffCom* device at device id *comDeviceId*.
	
.. function:: deregisterComDevice
   
   deregisters the currently registered *BeckhoffCom* device.

.. function:: plcUpdate

   called when PLC sends updates of values that have changed between PLC cycles
   
	
Hardware Granularity vs. Device Granularity
===========================================

What might be considered a single piece of hardware might actually be controlled via
multiple Karabo devices, e.g. when implementations for subcomponents such as a motor,
temperature sensor etc. already exist. 

In such cases the existing Beckhoff devices should be used. Middle-layer devices can then
be employed to present an interface to the complete hardware to the user, and assure 
coordination of individual components.

Vice versa, a Beckhoff controller combining multiple input channels, e.g. for temperature
sensors, and represented by a single device, should be subdivided into multiple
middle-layer devices if the channels interface to distinctive hardware entities.


State-Following
===============

As has been introduced in Sections :ref:`general_concepts_simple_state_machine` and 
Section :ref:`states` Karabo devices usually only implement a simplified state machine,
i.e. state transition rules are not checked in software; rather it is assumed that
only valid state transitions are propagated by the hardware.

Consequently, it is up to the device progammer to check for the validity of hardware
states and software state follow-up. For Beckhoff devices a "mixed-mode" is implemented:
the state machine of the general states as given in the following table is implemented
using a full finite state machine model and transition rules are fully enforced by this.

================ =============== ===================
**Origin State** **Event**       **Resulting State**
---------------- --------------- -------------------
Disconnected     ConnectEvent    Connected
Connected        DisconnectEvent Disconnected
Connected        ErrorEvent      Error
Error            ErrorEvent      Error
Error            ResetEvent      Connected
Error            DisconnectEvent Disconnected
================ =============== ===================

This top level state machine automatically transitions to a connected state machine upon
connection:

================ ================== ===================
**Origin State** **Event**          **Resulting State**
---------------- ------------------ -------------------
Normal           SafeEvent          Safe
Normal           HardwareErrorEvent HardwareError
Safe             SafeEvent          Safe
Safe             NormalEvent        Normal
Safe             HardwareErrorEvent HardwareError
HardwareError    HardwareResetEvent Normal
HardwareError    HardwareErrorEvent HardwareError
================ ================== ===================


In the *Normal* state, the specific Beckhoff device then overwrites the state by
decoding
the bits indicating the hardware state. Therefore, all specific Beckhoff devices
need to implement the *decodeHardwareState* hook, as explained earlier.
Below is an example of another hook for a gauge device:

.. code-block:: C++

    std::string BeckhoffMaxiGaugeController::decodeHardwareState
        (const std::bitset<32>& hardwareStatusBitField) {
        
        // Decode the bits into string, e.g.
        // NOTE: Error handling is done in the base class
        if (hardwareStatusBitField.test(12)) return "Started";
        return "Stopped";
    }

    
Overall device vs. hardware integrity is thus assured by the full state machine,
while the hardware state decoding allows for application-specific 
functionality. 

.. _regulated_mode:

Regulated-Mode on Hardware
==========================

See Section :ref:`open_closed_loop` on how target value following should be implemented
on the device software side.

The *onTarget* attribute introduced in this section should automatically be set by
the BeckhoffDevice if the onTarget flag is passed for a given property.

.. todo::

	Implement the latter statement.

Middle-layer Devices
====================

