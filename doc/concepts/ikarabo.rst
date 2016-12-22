*******
iKarabo
*******

iKarabo is Karabo's command line interface (CLI), running in the iPython prompt. For Karabo it
was chosen to not implement yet another macro language, but instead leverage the power
and conciseness of iPython. Although a full-fledged programming language is used at the
core, iKarabo's policies state that on the CLI, generally only *one-liners* should be used.
This especially means that users should not start implementing state-ful objects on the 
command line. Such tasks requiring state-awareness or which are generally more complex
should be implemented as middle-layer devices or macros.

Given the just mentioned policy restrictions, iKarabo should be used interact with
instantiated devices, i.e. open a valve, drive a motor, or to initiate scans without the
need for extensive programming. 

Reading and Writing a Property with iKarabo
===========================================

Properties are accessible through the device property interface in iKarabo, in a very
similar fashion to middle-layer devices. In a first step you need to create a proxy
for the Device from which you need to access properties. Its properties are then
directly available as members of the proxy:

.. code-block:: python

	sampleStageX = connectDevice("sampleStageX")
	sampleStage.position
	>> 10
	
Changing properties is simply done by assignment to the proxy:

.. code-block:: python

	sampleStageX = connectDevice("sampleStageX")
	sampleStage.target = 20
	
Upon assignment it is checked if the property is writable (with the user's access rights)
and bounds and validity checks are performed. If any of these criteria are not matched
an error is raised.

.. note::

	iKarabo allows for auto-completion of instance ids. When you start typing the instance
	id, press *tab* to see a list of available instances matching the characters you have
	already typed.

Purely out of convenience reasons iKarabo allows for direct property interaction without
proxy objects:

.. code-block:: python

	client().get("sampleStageX", "position)
	>> 10
	
	client().set("sampleStageX", "target", 20)
	
The above code will show the same results as the previous examples using device proxies.
However, it is strongly recommended to use proxy-objects, when you interact with a device
more than once as the connection the the device is persisted through the lifetime of the
proxy or until you disconnect it. Additionally, proxy devices let you access additional, 
sometimes crucial information:

.. code-block:: python

	sampleStageX = connectDevice("sampleStageX")
	sampleStage.target.prefixAndUnit()
	>> metric prefix: milli, unit: meter
	
.. todo::

	Martin, please check if access to prefix and unit information can be implemented in
	this way. In my opinion I think is necessary to have from the beginning.
	
.. todo::
	
	Having the convenience wrappers for get, set violates the clean interface provided
	by proxy devices. I think they are necessary, in the clear boundaries set, because
	users will request not always having to instantiate a proxy to quickly access a
	property in the system.

Performing a Hardware-Action with iKarabo
=========================================

In iKarabo hardware actions should be performed through the device proxy interface:

.. code-block:: Python

	gateValve = connectDevice("gateValve")
	gateValve.close()
	>> Closing

If you plan on only a single quick interaction you can use the convenience interface:

.. code-block.: Python

   execute("gateValve", "close")
   >> Closing

.. todo::

	Not sure if this already exists: successful completion of a slot acting on hardware
	should in my opinion always result in returning the device state after execution.


Transitioning to a new Set-Value with iKarabo
=============================================

Transitioning to a new set-value is generally a two-step process in iKarabo, and as most
other things involves using a device proxy. For example: moving a stage to a new position
requires:

- setting the target value by assigning to the corresponding property on the proxy
- executing the move command on the device proxy

.. code-block:: python

	sampleStageX = connectDevice("sampleStageX")
	sampleStage.target = 25
	sampleStage.move()
	>> Stopped
	
There is one exception to this rule. Devices acting in a regulated mode, as described in
Section :ref:`regulated_mode` may have properties which will directly trigger a hardware
change upon assignment:

.. code-block:: python

	sampleStageX = connectDevice("sampleStageY")
	>> sampleStageY.target is in following mode
	sampleStage.target = 25
	>> Following
	
.. todo::

	This concept should be briefly discussed with AE and the USPs, checking if *they* deem
	it a requirement. Further discussions should result in whether connecting to a device
	will print out the regulated properties - User's need to made aware somehow, as 
	assignment to these properties will violate the more general concept. Again the return
	value of assignment to a regulated property should be the resulting device state.
	
	To discuss: Burkhard, USPs, AE, Andreas Beckmann. To discuss for implementation: Martin

Initiating a Scan with iKarabo
==============================

Scan are a recurring task in beam-line environments. Especially during experiment setup 
and optimization a possibility of *light-weight* data-taking without having to go through
a full run-initialization is preferable. In iKarabo such functionality is exposed by
interfacing to the run configuration through a special acquisition device, which will
take care of configuring the DAQ system. This device is used in conjunction with *scan 
devices*. These are middle-layer devices implementing a certain type of scan and with 
master-slave hooks for interaction with the DAQ system.

In the below example a motor is interface with a scan device. The *SimpleRunAcquisition*
device is then configured with a list of data sources to record, as defined by the 
instance id and the DAQ behavior. Available behaviors are

========= ===========================================================================
ignore    ignore this device for DAQ, probably not commonly needed
readonly  record all read-only properties of the device, usually hardware read value
recordall record all properties of the device, including set-values. 
========= ===========================================================================

.. note:: 

	If you need to compare the set, i.e. target value of a property with the actual value,
	be sure to select *recordall* as DAQ behaviour for this device.

.. note:: 
	
	If you omit the behavior key in the the sources definition the behavior will default
	to *recordall*.
	
.. note::

	Initiating such a scan will lead to temporary disabling of all online stream which
	rely on properties not defined in your sources list. Upon completion of the scan
	the previous run configuration will be restored and full associated monitoring will be
	available again.

.. code-block:: Python

	scanDevice = connectDevice("Motor-scan")
	scanDevice.start = 0
	scanDevice.stop = 10
	scanDevice.steps = 100

	#SimpleRunAcquisition is a wrapper to ITDM run management
	#providing a simplified interface for configuration. It also
	#triggers runs provides feedback to the scan devices calling it.
	scanAcquisition = connectDevice("SimpleRunAcquisition)
	sources = [ {"instance": "/a/b/c", "behaviour": readonly},
	  {"instance": "/a/b/d", "behaviour": recordall}]
	scanAcquisition.datasources = sources
	scanAcquisition.numTrains = 1000
	scanAcquisition.runController = "DAQ/runcontroller_1"

	scanDevice.measurementGroup = "SimpleRunAcquisition"
	scanDevice.start()
	

.. ifconfig:: includeDevInfo is True

	.. note::
	
	The implementation of the *SimpleRunAcquistion* device needs to take care that the
	previous run configuration can be restored by temporary storing of this config.

.. todo::

	Technically, it is possible to preserve all online streaming functionality by
	violating the principle that data on the stream outputs must be in the persisted
	file. One can discuss if for such a simple scan case we allow this.
	
	Discussants: ITDM, M. Kuster, Instruments

