.. _scpiDevice:

***************************
 How to write a SCPI device
***************************

Introduction
============

From `wikipedia
<http://en.wikipedia.org/wiki/Standard_Commands_for_Programmable_Instruments>`_:
'The Standard Commands for Programmable Instruments (SCPI) (often
pronounced "skippy") defines a standard for syntax and commands to use
in controlling programmable test and measurement devices.'


How to Create a SCPI Karabo Device
==================================

Create a python Karabo Device, called for example myScpiDevice:

.. code-block:: bash

    ./karabo new myScpiDevice controlDevices pythonDevice minimal MyScpiDevice

Go into the base directory of the project:

.. code-block:: bash

    cd packages/controlDevices/myScpiDevice

Add there a DEPENDS file like this:

.. code-block:: bash

    # type        category        package    tag
    dependency    dependencies    scpi       branches/1.1-1.5

Do not forget to add the DEPENDS file to the repository:

.. code-block:: bash

    svn add DEPENDS

Now you can open the source file, MyScpiDevice.py, for editing.


State Machines
==============

There are currently three state machines available for SCPI devices:
ScpiOkErrorFsm, ScpiOnOffFsm, ScpiStartStopFsm.

ScpiOkErrorFsm is the simplest one. It has only three states:
Disconnected, Ok and Error.

ScpiOnOffFsm has three top states: Disconnected, Ok and Error, and the
Ok state contains two sub-states, Ok.On and Ok.Off. This state machine
is suitable for example for power supplies.

ScpiStartStopFsm has three top states: Disconnected, Ok and Error, and
the Ok state contains two sub-states, Ok.Started and Ok.Stopped. This
state machine is suitable for example for measurement devices.


.. _scpi-writing-a-device:

Writing a Device
================


Writing an Ok/Error Device
--------------------------

The state machine to be used is ScpiOkErrorFsm. The device is very
simple and should look like:

.. code-block:: python

    #!/usr/bin/env python
    
    __author__="john.smith@xfel.eu"
    __date__ ="June  8, 2015, 01:53 PM"
    __copyright__="Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved."

    from karabo.bound import KARABO_CLASSINFO, launchPythonDevice
    from scpi.scpi_device_2 import ScpiDevice2, ScpiOkErrorFsm


    @KARABO_CLASSINFO("MyScpiDevice", "1.5")
    class MyScpiDevice(ScpiDevice2, ScpiOkErrorFsm):
    
        def __init__(self, configuration):
            # always call superclass constructor first!
            super(MyScpiDevice,self).__init__(configuration)
            
            self.commandTerminator = "\r\n" # The command terminator
        
        @staticmethod
        def expectedParameters(expected):
            ( 
            # Fill here with the list of expected parameters
            )
    
    if __name__ == "__main__":
        launchPythonDevice()

The SCPI parameters can be accessed using Karabo expected parameters
(see the :ref:`scpi-expected-parameters` Section for details).


Writing an On/Off Device
------------------------

The state machine to be used is ScpiOnOffFsm. The device should look
pretty much like the Ok/Error one, except for the state machine to be
used. There is also a hook, followHardwareState, which can be used to
force the Karabo device to follow the hardware state.

.. code-block:: python

    #!/usr/bin/env python
    
    __author__="john.smith@xfel.eu"
    __date__ ="June  8, 2015, 01:53 PM"
    __copyright__="Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved."
    
    from karabo.bound import KARABO_CLASSINFO, launchPythonDevice
    from scpi.scpi_device_2 import ScpiDevice2, ScpiOnOffFsm


    @KARABO_CLASSINFO("MyScpiDevice", "1.5")
    class MyScpiDevice(ScpiDevice2, ScpiOnOffFsm):
    
        def __init__(self, configuration):
            # always call superclass constructor first!
            super(MyScpiDevice,self).__init__(configuration)
            
            self.commandTerminator = "\r\n" # The command terminator
        
        @staticmethod
        def expectedParameters(expected):
            (
            # Fill here with the list of expected parameters
            )
    
        def followHardwareState(self):
            # You can use this hook to follow the hardware state:
            # just call self.followOn() and self.followOff()
            pass
    
    if __name__ == "__main__":
        launchPythonDevice()

In the followHardwareState() method you can use self.followOn() and
self.followOff() to force the device to follow the hardware state,
without executing any action.


Writing a Start/Stop Device
---------------------------

The state machine to be used is ScpiStartStopFsm. There are three
additional hooks: preAcquisition() will be executed when entering the
startedState, postAcquisition() will be executed when leaving the
startedState, processAsyncData(data) will be executed in startedState,
each time data are received asynchronously. The device should look
like

.. code-block:: python

    #!/usr/bin/env python
    
    __author__="john.smith@xfel.eu"
    __date__ ="June  9, 2015, 01:55 PM"
    __copyright__="Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved."
    
    from karabo.bound import KARABO_CLASSINFO, launchPythonDevice
    from scpi.scpi_device_2 import ScpiDevice2, ScpiStartStopFsm


    @KARABO_CLASSINFO("MyScpiDevice", "1.5")
    class MyScpiDevice(ScpiDevice, ScpiStartStopFsm):
    
        def __init__(self, configuration):
            # always call superclass constructor first!
            super(MyScpiDevice,self).__init__(configuration)
            
            self.commandTerminator = "\r\n" # The command terminator
        
        @staticmethod
        def expectedParameters(expected):
            ( 
            # Fill here with the list of expected parameters
            )
    
        def followHardwareState(self):
            # You can use this hook to follow the hardware state:
            # just call self.followStarted() and self.followStopped()
            pass
    
        def processAsyncData(self, data):
            # In this hook you can process data received asynchronously when in Ok.Started state
            pass
    
        def preAcquisition(self):
            # This will be excecuted before starting acquisition
            pass
    
        def postAcquisition(self):
            # This will be excecuted after acquisition has been stopped
            pass
    
    if __name__ == "__main__":
        launchPythonDevice()

In the followHardwareState() method you can use self.followStarted()
and self.followStopped() to force the device to follow the hardware
state, without executing any action.


How to read/write parameters from/to the instrument
---------------------------------------------------

Each parameter on the instrument you want to have available in the
Karabo device, must have a corresponding expected parameter in the
Karabo device. The expected parameter must be tagged as 'scpi'. Please
have a look at the :ref:`scpi-expected-parameters` Section for the
details.


.. _scpi-expected-parameters:

Expected Parameters
===================


.. _scpi-tags:

Tags
----

* **'scpi'** tag: Parameters to be read from (written to) the SCPI
  instrument must have the 'scpi' tag.

* **'readOnConnect'** and **'writeOnConnect'** tags: Parameters having
  the 'readOnConnect' (respectively 'writeOnConnect') flag will be
  read from (written to) the instrument when the Karabo device
  connects to it.

* **'poll'** tag: Parameters having the 'poll' tag will be polled
  periodically. The poll interval is a parameter of the base class.


The "sendOnConnect" Parameter
-----------------------------

Commands to be sent to the instrument when the Karabo device connects
to it (for example some initial configuration), can be listed in the
``__init__`` function; for example

.. code-block:: python

    self.sendOnConnect = ['TRIG:LEV 10', 'TRIG:SOURCE EXT', 'SYST:COMM:SER:BAUD 19200']

These commands will be sent before the expected parameters with
"writeOnConnect" tag (see :ref:`scpi-tags` Section).
 

Aliases
-------

The SCPI commands and queries corresponding to writing and reading any
parameter must be written in the parameter alias. Different fields in
the alias have to be separated by semicolons (;) or a different
separator (as explained in :ref:`scpi-alias-separator` Section). For
example

.. code-block:: python

    INT32_ELEMENT(expected).key("resolutionMode")
            .tags("scpi poll")
            .alias(">S1H {resolutionMode};E0;>S1H?;S1H:{resolutionMode:d};")
            .displayedName("Current Resolution Mode")
            .description("Set the current resolution mode (0=normal 1=high resolution).")
            .assignmentOptional().defaultValue(0)
            .options("0 1")
            .allowedStates("Ok.On Ok.Off")
            .reconfigurable()
            .commit(),

The first field in the alias contains the set command (ie >S1H) and
its parameters (ie {resolutionMode}) for the resolutionMode. This
string will be parsed, and {resolutionMode} will be replaced by the
configuration value corresponding to the key. The second field (ie E0)
is the expected reply to the set command; it is also parsed to extract
parameters (none in this example).

The third field contains the query command (ie >S1H?) and its
parameters (none). The fourth field (ie {resolutionMode:d}) is the
expected reply to the query; it is parsed and resolutionMode is
extracted as integer (d). The parsing is done by using the python
parse package (see `documentation
<https://pypi.python.org/pypi/parse>`_), therefore all types defined
there can be used:

+------+---------------------------------------------------------+----------+
| Type | Characters Matched                                      | Output   |
+======+=========================================================+==========+
| w    | Letters and underscore                                  | str      |
+------+---------------------------------------------------------+----------+
| W    | Non-letter and underscore                               | str      |
+------+---------------------------------------------------------+----------+
| s    |  Whitespace                                             | str      |
+------+---------------------------------------------------------+----------+
| S    | Non-whitespace                                          | str      |
+------+---------------------------------------------------------+----------+
| d    | Digits (effectively integer numbers)                    | int      |
+------+---------------------------------------------------------+----------+
| D    | Non-digit                                               | str      |
+------+---------------------------------------------------------+----------+
| n    | Numbers with thousands separators (, or .)              | int      |
+------+---------------------------------------------------------+----------+
| %    | Percentage (converted to value/100.0)                   | float    |
+------+---------------------------------------------------------+----------+
| f    | Fixed-point numbers                                     | float    |
+------+---------------------------------------------------------+----------+
| e    | Floating-point numbers with exponent e.g. 1.1e-10,      | float    |
|      | NAN (all case insensitive)                              |          |
+------+---------------------------------------------------------+----------+
| g    | General number format (either d, f or e)                | float    |
+------+---------------------------------------------------------+----------+
| b    | Binary numbers                                          | int      |
+------+---------------------------------------------------------+----------+
| o    | Octal numbers                                           | int      |
+------+---------------------------------------------------------+----------+
| x    | Hexadecimal numbers (lower and upper case)              | int      |
+------+---------------------------------------------------------+----------+
| ti   | ISO 8601 format date/time                               | datetime |
|      | e.g. 1972-01-20T10:21:36Z (“T” and “Z” optional)        |          |
+------+---------------------------------------------------------+----------+
| te   | RFC2822 e-mail format date/time                         | datetime |
|      | e.g. Mon, 20 Jan 1972 10:21:36 +1000                    |          |
+------+---------------------------------------------------------+----------+
| tg   | Global (day/month) format date/time                     | datetime |
|      | e.g. 20/1/1972 10:21:36 AM +1:00                        |          |
+------+---------------------------------------------------------+----------+
| ta   | US (month/day) format date/time                         | datetime |
|      | e.g. 1/20/1972 10:21:36 PM +10:30                       |          |
+------+---------------------------------------------------------+----------+
| tc   | ctime() format date/time                                | datetime |
|      | e.g. Sun Sep 16 01:03:52 1973                           |          |
+------+---------------------------------------------------------+----------+
| th   | HTTP log format date/time                               | datetime |
|      | e.g. 21/Nov/2011:00:07:11 +0000                         |          |
+------+---------------------------------------------------------+----------+
| ts   | Linux system log format date/time                       | datetime |
|      | e.g. Nov 9 03:37:44                                     |          |
+------+---------------------------------------------------------+----------+
| tt   | Time e.g. 10:21:36 PM -5:30                             | time     |
+------+---------------------------------------------------------+----------+

In addition, two extra types can be used for the SCPI devices:

+------+---------------------------------------------------------+----------+
| Type | Characters Matched                                      | Output   |
+======+=========================================================+==========+
| p    | All printable characters                                | str      |
+------+---------------------------------------------------------+----------+
| P    | All non-printable characters                            | str      |
+------+---------------------------------------------------------+----------+


.. _scpi-alias-separator:

The "aliasSeparator" Parameter
------------------------------

The separator for the fields in the alias is by default the semicolon (;), but can be changed to a different one in the __init__ function; for example:

.. code-block:: python

    self.aliasSeparator = "|"

will change it to the pipe character (\|).


The "terminator" Parameter
--------------------------

The command terminator -  to be used in the communications between the Karabo device and the SCPI instrument - can be set in two different ways. For a given device, the command terminator is usually known and fixed, therefore should be hard-coded in the Karabo device. This can be done by adding a line like this to the __init__ function:

.. code-block:: python

    self.commandTerminator = "\r\n" # The command terminator

The second way to set the command terminator is by adding the "terminator" expected parameter. This should be done for "generic" devices, for which different terminators should be available at instantiation time. For example:

.. code-block:: python

    # Re-define default value and options
    STRING_ELEMENT(expected).key("terminator")
            .displayedName("Command Terminator")
            .description("The command terminator.")
            .assignmentOptional().defaultValue("\\n")
            .options("\\n \\r \\r\\n")
            .init()
            .commit(),

If the terminator is not set in the Karabo device, the default one will be used for communications with the SCPI instrument: "\\n".


.. _scpi-timeout-parameter:

The "scpiTimeout" Parameter
---------------------------

The default scpi communication timeout used in the base class is 1
second. This value is normally ok, but some instruments (eg the
agilentMultimeterPy) may need a longer time to give back a
measurement.

The scpi timeout (in seconds) can be redifined in __init__ with
something like:

.. code-block:: python

    self.scpiTimeout = 5.0 # New timeout value in seconds
 
A second way to set it is by adding the "scpiTimeout" expected
parameter. In this way the timeout can be changed during the lifetime
of the Karabo device. For example:

.. code-block:: python

    FLOAT_ELEMENT(expected).key("scpiTimeout")
            .displayedName("SCPI Timeout")
            .description("The scpi communication timeout.")
            .unit(Unit.SECOND)
            .assignmentOptional().defaultValue(1.0)
            .reconfigurable()
            .commit(),
 
If the scpi timeout is not set in the Karabo device, the default value of 1 s will be used.


The "socketTimeout" Parameter
-----------------------------

The default TCP socket timeout used in the base class is 1 second.
Similarly to the scpi communication timeout, also the TCP socket
timeout can be redefined, either in the __init__ by doing

.. code-block:: python

    self.socketTimeout = 2.0 # New timeout value in seconds

or by defining a "socketTimeout" element in the expected parameters.


On/Off (and Start/Stop) Slots
-----------------------------

For On/Off (Start/Stop) devices, the on/off (start/stop) slots are
already defined in the state machines. What you have to do, is to set
the SCPI command in the slots's alias. For example, for the
start/stop:

.. code-block:: python

    # Define alias for the "start" slot
    OVERWRITE_ELEMENT(expected).key("start")
            .setNewAlias("INIT;;;;") # No query available
            .commit(),

    # Define alias for the "stop" slot
    OVERWRITE_ELEMENT(expected).key("stop")
            .setNewAlias("ABORT;;;;") # No query available
            .commit(),


Additional Slots (Command-like Parameters)
------------------------------------------

A SLOT_ELEMENT should be used for a SCPI command which is not
triggering a state change in the Karabo Device. This requires not only
to to add the expected parameter in the list:

.. code-block:: python

    @staticmethod
    def expectedParameters(expected):
        (
        # ...
        
        SLOT_ELEMENT(expected).key("statStart")
                .tags("scpi")
                .alias("CONF:STAT:START;;;;") # No query available
                .displayedName("Start Statistical Batch")
                .description("Terminates the current statistical batch and start a new one.")
                .allowedStates("Ok.Stopped")
                .commit(),
        
        # ...
        )

but also to register the slot,

.. code-block:: python

    def registerAdditionalSlots(self, sigslot):
        '''Register additional slots'''
        sigslot.registerSlot(self.statStart)

and to implement the corresponding function,

.. code-block:: python

    def statStart(self):
        '''Will start statistical batch'''
        
        try:
            self.sendCommand("statStart")
        except:
            # Re-raise exception
            raise


A Complete Example 
------------------

Here is a complete example of expected parameters for a Start/Stop device:

.. code-block:: python

      # Define alias for the "start" slot
      OVERWRITE_ELEMENT(expected).key("start")
              .setNewAlias("INIT;;;;") # No query available
              .commit(),

      # Define alias for the "stop" slot
      OVERWRITE_ELEMENT(expected).key("stop")
              .setNewAlias("ABORT;;;;") # No query available
              .commit(),

      # Re-define default value and options
      STRING_ELEMENT(expected).key("terminator")
                .displayedName("Command Terminator")
                .description("The command terminator.")
               .assignmentOptional().defaultValue("\\n")
                .options("\\n")
                .init()
                .commit(),

      STRING_ELEMENT(expected).key("handshake")
                .tags("scpi")
                .alias("SYST:COMM:HAND {handshake};;SYST:COMM:HAND?;{handshake:w};")
                .displayedName("Handshake")
                .description("Set the state of the message roundtrip handshaking.")
                .assignmentOptional().defaultValue("OFF")
                .options("OFF ON")
                .allowedStates("Ok.Stopped")
                .reconfigurable()
                .commit(),

      STRING_ELEMENT(expected).key("baudRate")
                .tags("scpi")
                .alias("SYST:COMM:SER:BAUD {baudRate};;SYST:COMM:SER:BAUD?;{baudRate:w};")
                .displayedName("Serial Baud Rate")
                .description("Set the transmit and receive baud rates on the RS-232 port.")
                .assignmentOptional().defaultValue("9600")
                .options("DEFAULT 9600 19200 38400 57600 115200")
                .allowedStates("Ok.Stopped")
                .reconfigurable()
                .commit(),

      INT32_ELEMENT(expected).key("errorCount")
                .tags("scpi poll")
                .alias(";;SYST:ERR:COUNT?;{errorCount:d};") # Only query available
                .displayedName("Error Count")
                .description("The number of error records in the queue.")
                .readOnly()
                .commit(),

      STRING_ELEMENT(expected).key("measureType")
                .tags("scpi writeOnConnect") # Write to h/w at initialization
                .alias("CONF:MEAS:TYPE {measureType};;CONF:MEAS:TYPE?;{measureType:w};")
                .displayedName("Measure Type")
                .description("Set the meter measurement mode (energy or power).")
                .assignmentOptional().defaultValue("J")
                .options("DEFAULT J W")
                .allowedStates("Ok.Stopped")
                .reconfigurable()
                .commit(),

      STRING_ELEMENT(expected).key("serialNumber")
                .tags("scpi readOnConnect") # Read from h/w at initialization
                .alias(";;SYST:INF:SNUM?;\"{serialNumber:p}\";") # Only query available
                .displayedName("Serial Number")
                .description("The serial number.")
                .readOnly()
                .commit(),


Polling Device Properties
=========================

All the expected parameters having the "poll" tag will be
automatically polled (see :ref:`scpi-tags` Section). The refresh
interval is given by the 'pollInterval' device parameter.

An immediate refresh can be triggered by the 'pollNow' command.

The list of parameters to be polled can be reconfigured be means of
the 'propertiesToPoll' property. For example, if you set it to
'handshake,baudRate' these two properties will be polled. The access
level for 'propertiesToPoll' property is expert.

A user's hook is also provided by the base class, allowing the
post-processing of the polled properties. For example, if you read
some temperature in Fahrenheit degrees and you want to display it in
Celsius, you can define two expected parameters, like in the
following:

.. code-block:: python

    FLOAT_ELEMENT(expected).key("temperature")
              .displayedName("Temperature")
              .description("Blah blah.")
              .unit(Unit.DEGREE_CELSIUS)
              .readOnly()
              .commit(),

    FLOAT_ELEMENT(expected).key("temperatureFahrenheit")
              .tags("scpi poll")
              .alias(";;GETTEMP;{temperatureHex:g};")
              .displayedName("Temperature Fahrenheit")
              .description("Blah blah.")
              .expertAccess() # Only visible to expert
              .readOnly()
              .commit(),


Then, you can postprocess the polled data this way:

.. code-block:: python

    def pollInstrumentSpecific(self):

        # 'temperatureFahrenheit' is a polled property
        tF = self.get('temperatureFahrenheit')

        # Convert temperatureFahrenheit into Celsius degrees
        tC = (tF - 32.) / 1.8

        # Set 'temperature' on the Karabo device, which is a derived property
        self.set('temperature', tC)


Preprocessing the incoming reconfiguration
==========================================

The base class provides a user's hook to preprocess the incoming
reconfiguration, before any command is sent to the instrument.  It can
be used for example when the instrument expects a parameter in some
unusual units, and you would like to allow the user to input the
parameter with a more standard unit.

This can be done by using two expected parameters, like in the following:

.. code-block:: python

    FLOAT_ELEMENT(expected).key("temperature")
              .tags("scpi")
              .alias("SETTEMP {temperatureHex};;;;")
              .displayedName("Temperature")
              .description("Blah blah.")
              .unit(Unit.DEGREE_CELSIUS)
              .reconfigurable()
              .commit(),

    STRING_ELEMENT(expected).key("temperatureHex")
              .displayedName("TemperatureHex")
              .description("Blah blah.")
              .expertAccess() # Only visible to expert
              .readOnly()
              .commit(),


and then by coding the relation between temperature and temperatureHex
in the preprocessConfiguration function,

.. code-block:: python

    def preprocessConfiguration(self, inputConfig):
        
        if inputConfig.has('temperature'):
            # Get temperature from inputConfig, change unit,
            # represent it as bytes
            temp = inputConfig.get('temperature') # eg -23.15
            temp100 = np.int16(100*temp) # -2315
            tempBytes = temp100.tostring() # b'\xf5\xf6'
            tempHex = '%02X%02X'%(tempBytes[0], tempBytes[1]) # 'F5F6'

            self.set('temperatureHex', tempHex)


Setting a new value for 'temperature' will make preprocessConfiguration called.
A new value for 'temperatureHex' will be set in the device, and only then
the command 'SETTEMP' will be sent to the instrument, with 'temperatureHex'
as an additional parameter.


Enabling the Heartbeat
======================

The scpi Karabo device can periodically send a heartbeat query to the
instrument. The default query is "*IDN?", which should be available
for all SCPI-compliant instruments. The sending of the heartbeat query
is by default disabled, but it can be enabled by setting the
'enableHeartbeat' property to True.

Once a reply to the query is received, it is published in the 'heartbeatReply'
property, and the 'heartbeatTime' is updated as well, with the current time.

The query can be changed in the 'heartbeatCommand' property, and also the time interval can
be changed in the 'heartbeatPeriod' property (default value is 5 s).


Sending an Arbitrary Command to the Instrument
==============================================

An arbitrary command (or query) can be sent to the instrument. To do
so, it is enough to write the command (or query) in the 'sendCommand'
device property. This property is expert access level.

Once a reply is received, it is published in the 'replyToCommand' property. If no
reply is received after the scpi timeout (see :ref:`scpi-timeout-parameter`),
'replyToCommand' is left empty.
