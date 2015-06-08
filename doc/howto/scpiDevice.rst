.. _scpiDevice:

***************************
 How to write a SCPI device
***************************

Introduction
============

From `wikipedia <http://en.wikipedia.org/wiki/Standard_Commands_for_Programmable_Instruments>`_: 'The Standard Commands for Programmable Instruments (SCPI) (often pronounced "skippy") defines a standard for syntax and commands to use in controlling programmable test and measurement devices.'


How to Create a SCPI Karabo Device
==================================

Create a python Karabo Device, called for example myScpiDevice::

    ./karabo new myScpiDevice controlDevices pythonDevice reuseFsm MyScpiDevice

Go into the base directory of the project::

    cd packages/controlDevices/myScpiDevice

Add there a DEPENDS file like this::

    # type        category        package    tag
    dependency    dependencies    scpi       branches/1.0-1.3

Open the source file in Netbeans for editing.


State Machines
==============

There are currently three state machines available for SCPI devices: ScpiOkErrorFsm, ScpiOnOffFsm, ScpiStartStopFsm.

ScpiOkErrorFsm is the simplest one. It has only three states: Disconnected, Ok and Error.

ScpiOnOffFsm has three top states: Disconnected, Ok and Error, and the Ok state contains two sub-states, Ok.On and Ok.Off. This state machine is suitable for example for power supplies.

ScpiStartStopFsm has three top states: Disconnected, Ok and Error, and the Ok state contains two sub-states, Ok.Started and Ok.Stopped. This state machine is suitable for example for measurement devices.


Writing an Ok/Error Device
==========================

The state machine to be used is ScpiOkErrorFsm. The device is very simple and should look like::

    !/usr/bin/env python
    
    __author__="john.smith@xfel.eu"
    __date__ ="June  8, 2015, 01:53 PM"
    __copyright__="Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved."
    
    from scpi.scpi_device_2 import *
    
    @KARABO_CLASSINFO("MyScpiDevice", "1.2 1.3")

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

The SCPI parameters can be accessed using Karabo expected parameters (more details in the following).


Writing an On/Off Device
========================

The state machine to be used is ScpiOnOffFsm. The device should look pretty much like the Ok/Error one, except for the state machine to be used. There is also a hook, followHardwareState, which can be used to force the Karabo device to follow the hardware state. ::

    !/usr/bin/env python
    
    __author__="john.smith@xfel.eu"
    __date__ ="June  8, 2015, 01:53 PM"
    __copyright__="Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved."
    
    from scpi.scpi_device_2 import *
    
    @KARABO_CLASSINFO("MyScpiDevice", "1.2 1.3")
    
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

In the followHardwareState() method you can use self.followOn() and self.followOff() to force the device to follow the hardware state, without executing any action.


Writing a Start/Stop Device
===========================

The state machine to be used is ScpiStartStopFsm. There are three additional hooks: preAcquisition() will be executed when entering the startedState, postAcquisition() will be executed when leaving the startedState, processAsyncData(data) will be executed in startedState, each time data are received asynchronously. The device should look like::

    !/usr/bin/env python
    
    __author__="john.smith@xfel.eu"
    __date__ ="June  9, 2015, 01:55 PM"
    __copyright__="Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved."
    
    from scpi.scpi_device_2 import *
    
    @KARABO_CLASSINFO("MyScpiDevice", "1.2 1.3")
    
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

In the followHardwareState() method you can use self.followStarted() and self.followStopped() to force the device to follow the hardware state, without executing any action.


Expected Parameters
===================


.. _tags-section:

Tags
----

Parameters to be read/written to the SCPI instrument must have the 'scpi' tag. If they have the 'readOnConnect' (respectively 'writeOnConnect') will be read from (written to) the instrument when the Karabo device connects to it. If they have the 'poll' tag, they will be polled regularly. The parameter tagged with the 'handshake' flag, if there, will be used to determine whether the handshaking message is turned on or off ("ON", "1", 1, True will be interpreted as handshaking is on; any other value as off).


The "sendOnConnect" Parameter
-----------------------------

Commands to be sent to the instrument when the Karabo device connects to it (for example some initial configuration), can be listed in the __init__ function; for example::

    self.sendOnConnect = ['TRIG:LEV 10', 'TRIG:SOURCE EXT', 'SYST:COMM:SER:BAUD 19200']

These commands will be sent before the expected parameters with "writeOnConnect" tag (see :ref:`tags-section` Section).
 

Aliases
-------

The SCPI commands and queries corresponding to writing and reading any parameter must be written in the parameter alias. Different fields in the alias have to be separated by semicolons (;) or a different separator (as explained in :ref:`alias-separator-section` Section). For example::

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

The first field in the alias contains the set command (ie >S1H) and its parameters (ie {resolutionMode}) for the resolutionMode. This string will be parsed, and {resolutionMode} will be replaced by the configuration value corresponding to the key. The second field (ie E0) is the expected reply to the set command; it is also parsed to extract parameters (none in this example).

The third field contains the query command (ie >S1H?) and its parameters (none). The fourth field (ie {resolutionMode:d}) is the expected reply to the query; it is parsed and resolutionMode is extracted as integer (d). Other allowed types are "w" (letters and underscores), "g" (integer, fixed point or floating point numbers). The python parse package is used for parsing: the complete list of types can be found in the `documentation <https://pypi.python.org/pypi/parse>`_.


.. _alias-separator-section:

The "aliasSeparator" Parameter
------------------------------

The separator for the fields in the alias is by default the semicolon (;), but can be changed to a different one in the __init__ function; for example::

        self.aliasSeparator = "|"

will change it to the pipe character (|).


The "terminator" Parameter
--------------------------

The command terminator -  to be used in the communications between the Karabo device and the SCPI instrument - can be set in two different ways. For a given device, the command terminator is usually known and fixed, therefore should be hard-coded in the Karabo device. This can be done by adding a line like this to the __init__ function::

    self.commandTerminator = "\r\n" # The command terminator

The second way to set the command terminator is by adding the "terminator" expected parameter. This should be done for "generic" devices, for which different terminators should be available at instantiation time. For example:: 

    # Re-define default value and options
    STRING_ELEMENT(expected).key("terminator")
            .displayedName("Command Terminator")
            .description("The command terminator.")
            .assignmentOptional().defaultValue("\\n")
            .options("\\n \\r \\r\\n")
            .init()
            .commit(),

If the terminator is not set in the Karabo device, the default one will be used for communications with the SCPI instrument: "\\n".


The "socketTimeout" Parameter
-----------------------------

The socket read/write timeout (in seconds) can be redifined in __init__ with something like::

    self.socketTimeout = 5.0
 
A second way to set it is by adding the "socketTimeout" expected parameter. In this way the timeout can be changed during the lifetime of the Karabo device. For example::

    FLOAT_ELEMENT(expected).key("socketTimeout")
            .displayedName("Socket Timeout")
            .description("The socket timeout.")
            .unit(Unit.SECOND)
            .assignmentOptional().defaultValue(1.0)
            .reconfigurable()
            .commit(),
 
If the socket timeout is not set in the Karabo device, the default value of 1 s will be used.

This value is normally ok, but some instruments (eg agilentMultimeterPy) need longer time to give back data.


On/Off (and Start/Stop) Slots
-----------------------------

For On/Off (Start/Stop) devices, the on/off (start/stop) slots are already defined in the state machines. What you have to do, is to set the SCPI command in the slots's alias. For example, for the start/stop::

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

A SLOT_ELEMENT should be used for a SCPI command which is not triggering a state change in the Karabo Device. This requires not only to to add the expected parameter in the list::

    @staticmethod
    def expectedParameters(expected):
        (
        # ...
        
        SLOT_ELEMENT(expected).key("statStart")
                .tags("scpi")
                .alias("CONF:STAT:START;;;;OK") # No query available
                .displayedName("Start Statistical Batch")
                .description("Terminates the current statistical batch and start a new one.")
                .allowedStates("Ok.Stopped")
                .commit(),
        
        # ...
        )

but also to register the slot, ::

    def registerAdditionalSlots(self, sigslot):
        '''Register additional slots'''
        sigslot.registerSlot(self.statStart)

and to implement the corresponding function, ::

    def statStart(self):
        '''Will start statistical batch'''
        
        try:
            self.sendCommand("statStart")
        except:
            # Re-raise exception
            raise


A Complete Example 
------------------

Here is a complte example of expected parameters for a Start/Stop device::

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
                .tags("scpi handshake") # This parameter tells whether handshaking is ON or OFF
                .alias("SYST:COMM:HAND {handshake};;SYST:COMM:HAND?;{handshake:w};OK")
                .displayedName("Handshake")
                .description("Set the state of the message roundtrip handshaking.")
                .assignmentOptional().defaultValue("OFF")
                .options("OFF ON")
                .allowedStates("Ok.Stopped")
                .reconfigurable()
                .commit(),

      STRING_ELEMENT(expected).key("baudRate")
                .tags("scpi")
                .alias("SYST:COMM:SER:BAUD {baudRate};;SYST:COMM:SER:BAUD?;{baudRate:w};OK")
                .displayedName("Serial Baud Rate")
                .description("Set the transmit and receive baud rates on the RS-232 port.")
                .assignmentOptional().defaultValue("9600")
                .options("DEFAULT 9600 19200 38400 57600 115200")
                .allowedStates("Ok.Stopped")
                .reconfigurable()
                .commit(),

      INT32_ELEMENT(expected).key("errorCount")
                .tags("scpi poll")
                .alias(";;SYST:ERR:COUNT?;{errorCount:d};OK") # Only query available
                .displayedName("Error Count")
                .description("The number of error records in the queue.")
                .readOnly()
                .commit(),

      STRING_ELEMENT(expected).key("measureType")
                .tags("scpi writeOnConnect") # Write to h/w at initialization
                .alias("CONF:MEAS:TYPE {measureType};;CONF:MEAS:TYPE?;{measureType:w};OK")
                .displayedName("Measure Type")
                .description("Set the meter measurement mode (energy or power).")
                .assignmentOptional().defaultValue("J")
                .options("DEFAULT J W")
                .allowedStates("Ok.Stopped")
                .reconfigurable()
                .commit(),

      STRING_ELEMENT(expected).key("serialNumber")
                .tags("scpi readOnConnect") # Read from h/w at initialization
                .alias(";;SYST:INF:SNUM?;\"{serialNumber}\";OK") # Only query available
                .displayedName("Serial Number")
                .description("The serial number.")
                .readOnly()
                .commit(),
