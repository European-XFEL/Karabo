********
Policies
********

Signals and Slots
=================

This section describes the policies implemented for the signal and slot communication layer.

- All signals and slots are uniquely defined by an instanceId (string) and a function name (string)
- Signals and slots use the message broker for transport and routing
- In a JMS sense signals are producers and slot are consumers
- The connection information between a signal and a slot is kept with the signal (it keeps all instanceIds and function names of the connected slots)
- The connected slots information is shipped in the header of the JMS message upon an signal emit
- The correct routing of a connected signal slot pair is done using JMS selectors
- The slot will always select for messages that contain the slots own instanceId, i.e. on the broker the selection is done on an instance level
- The final function routing is done on the slot instance locally
- Messages are consumed by an extra thread into a local message queue of the slot instance
- A configurable number of threads (default 2) pops the messages from the queue and trigger the final slot function call-back
- A connect between a signal and slot in principle looks like connect(signalInstanceId, signalFunction, slotInstanceId, slotFunction)
- Establishing connection can be done in two types: NO_TRACK, TRACK
- In the NO_TRACK case, after the connection was established no further information about existence is tracked nor reported, lost connections are not re-established, 
- In the TRACK case the connecting partners are automatically connected once the instances are availabe, this allows to connect to not-yet existing slots/signals 
- The connect call itself returns a boolean indicating whether the connection could be established or not. The call has an internal timeout of 1 second.  
- Besides the asynchronous signal and slot communication pattern 4 other exists
  - call (aynchronous, fire and forget)
  - request/reply (synchronous with timeout)
  - requestNoWait / reply to other slot (asynchronous)
  - request / reply on provided callBack (asynchronous)

 





Commands, Properties and State
==============================

Any regular device consists of properties and commands and once instanatiated must always be in some finite state.

The concept of commands, properties and state are implemented on top of the lower-level communication layer (signals and slots).

Properties

- Properties are key/value pairs and are internally stored in a dictionary structure (Hash) by the device base class.
- One should think about properties just like regular member variables of the device class with the difference that they are (in general) remotely readable and writable.
- That said, properties have a name (called "key" in Karabo) and a value.
- Setting a property within the device code not only updates the value locally on the device instance but at the same time publishes it to the broker. 
- Publishing of properties is realized via a signal (signalChanged) that ships one or more key/value pairs to the broker. Any time a property is set, a timestamp is associated (using attributes).
- Setting a property remotely results in a call to a single slot (slotReconfigure) which receives one or more key/value pairs. Typically this slot call is wrapped into convenience functions (widgets) on the remote clients. 
- Setting and getting properties on the device theoretically could happend concurrently, but is guaranteed to be thread-safe by the base class.
- If a property is set from remote, the device must always reply with a tuple. The first value must be a boolean indicating success, the second should carry a text message in case of failure.

Commands 

- One should think about commands like regular member functions with no argument
- That said, a command only has a name (called "key" in Karabo) and no value (that limitation is conceptional, slots in general can have up to four arguments)
- Within the device code, commands can be called just like any other regular member function
- Calling a command remotely is realized through a slot call (each command is one slot) which must have the same (string-)key name as the function name
- Any command function implementation must update the device state (even if its the same state as before)
- If a command is called from remote, the device must always reply with a tuple. The first value must be a boolean indicating success, the second should carry the state or the error message in case of success of failure, respectively.

State

- The state is a special property that is published using its own signal (signalStateChanged). This is important to apply different message policies (should never be dropped) in contrast to regular properties.
- The state property always must be named "state"


Devices can have so-called node elements, in which commands and properties can be nested. Sometimes it makes sense to understand an entire node as a sub-device. 
In this case each node must at least contain an own state property and may have additional properties and commands. The addressing must always include the node name for disambuigation.


Macros working with devices
===========================

Working with devices that need some time to reach a target (can be motor position, voltage, temperature):

with getDevice("SomeDevice") as dev:

   dev.currentPosition = 20 # Set the target property

   dev.move() # Move to the target

   waitUntil(lambda: dev.state == "Stopped") # Wait until target reached










