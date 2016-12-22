.. _run_configuration:

*****************
Run Configuration
*****************

Data Sources
============

The atomic unit of run configuration is the data source. All slow
control properties of a device, as defined in its expected parameters section,
constitute a single data source. Specifically, these are all properties which
update through messaging over the centralized message broker. Slow control
data sources are called *control data*.

Additionally, every pipelined processing output channel of a device is
considered a separate data source. These channels are expected to output
data train-wise and follow the structure definition as given in [to be added].
Output channels are defined as *instrument data*.

Run Configuration
=================

Run configuration is performed on three levels in XFEL/Karabo:

1. On the first level group-wise configuration is performed. Groups in this
   sense are e.g. a specific large detector, the sample environment, or other
   logically connected devices. They may or may not map directly to component
   groups as specified in the naming convention.
   The group level configuration has two configuration fields: one
   expert-accessible, which system integrators can use to define data sources
   that should always be persisted; and one user configurable, where users
   can configure additional data sources. For either field data sources are
   added by dropping device instances from the manager view onto the
   configuration widget. The entries are automatically expanded to include
   output channels as separate instrument data sources in the list and the
   device as a control data source.
   For each data source the following attributes are available:

   * Type [control/instrument]: a read-only attribute specifying if the data
     source is considered instrument or control data.

   * Behavior [ignore/read-only/record-all]: determines which slow control
     properties are to be recorded. In ignore-mode a static snap-shot of the
     device properties at the begin of the run is recorded. In read-only mode
     updates to read-only properties are recorded in addition to the init-mode
     snapshot. Record-all finally records all properties in addition to the
     init-snapshot. Selected Karabo internal properties, as defined in the
     device base class are omitted from all records. This setting has no
     effect for instrument data.

   * Output to monitor [true/false]: if selected data will be output to the
     online pipeline outputs in the DAQ's monitoring and recording states.

.. figure:: images/gui_run_group_config.png
   :alt: gui_run_group_config.png

   A group configuration scene used for run configuration.The top table shows
   expert configurations for mandatory data sources, the bottom table user
   selectable data sources. Note that new sources can be dropped to both
   tables from the navigation panel. The device at *SASE1/SPB/SAMP/INJ_CAM_1*
   has been automatically expanded to *control* and *instrument* parameters.

2. Component level run configuration is aggregated in a RunConfigurator device,
   of which a single instance exists within each Karabo domain. This device lists
   the available group configurations, and allows users to select the
   ones to include in the overall run configuration. For each group
   configuration the user/system integrator is expected to also specific the
   data aggregator, which will persist this source. Choices are given through
   a drop down field, which lists all all available aggregator devices
   in the current domain's system topology.

   All selected component configurations are expanded by the device into a flat
   list of sources, available through a separatefield, by using the following
   set of rules:

   * data sources in each component configuration are considered uniquely
     identifiable

   * data sources appearing in multiple component configurations are merged
     such that the most inclusive behavior is chosen. Here *record-all* is
     considered more inclusive than *read-only* is more inclusive than *ignore*.
     For the monitoring flag, *monitor out* enabled is considered more
     inclusive.

   * for all sources in a group configuration the data aggregator
     assigned to this group configuration is used as the default aggregator
     for source in the expanded group. Users may however chose a different
     aggregator for individual sources afterwards. This is e.g. useful if
     a single source within a group is known to produce high data rates.

   By default all user selectable sources are selected to be used when the
   group configuration is added to the overall run configuration. Users may
   deselect individual source from the flattened list.

   This list is published to the pc layer devices in a modified form
   (described below) upon user request. During this step any deselected data
   sources are filtered out.


.. figure:: images/gui_run_configurator.png
   :alt: gui_run_configurator.png

   The run configurator panel. The top table lists the group configurations
   available in this Karabo domain. The table to its right gives an overview
   of all source contained in a particular group. The bottom panel gives
   the evaluated composed list of source from all groups. Users may drag
   and drop additional sources to this list from the navigation view. Clicking
   *Push to DAQ* updates the run configuration on the run controller.

The modified form is a Hash of the following structure:

.. graphviz::

    digraph g {
        rankdir=LR;
        ordering=out;
        node [shape=box];
        ".....";
        "aggregatorB";
        "aggregatorA";
        "aggregatorA" -> "deviceId1";
        "deviceId1" -> "propertyA";
        "deviceId1" -> "propertyB";
        "deviceId1" -> "propertyC";
        "propertyC" -> "propertyC1";
        "propertyC" -> "propertyC2";
        "propertyC" -> "...";
        "aggregatorA" -> "deviceId2";
        "aggregatorA" -> "....";
        "aggregatorB" -> "deviceId3";


    }


Data sources may be assigned to only a single aggregator.  For each property
entry the following attributes are inferred from the Karabo device Schema:

* displayed name
* description
* alarm bounds
* unit
* metric prefix
* pipeline output

Property entries do not contain any time-related information. They are typed
according to their Karabo type and have an undefined value
(frequently zero, or a zero-length vector).

For each device entry the following attributes are added:

* class id/name
* version

On the property level nested Hashes are used for nested property structures.
The device id is not nested by domain, e.g. it is a single string key of form
`DOMAIN/CLS/DEVICE_A_B` is used. Properties that refer to output channels
additionally have a `pipelineOutput` attribute set to `true`.


3. The run controller on the PC-layer uses this hash to reconfigure the
   aggregators (and other services) upon receiving a newly pushed configuration.
   Pushing is accomplished via a signal that the runConfigurator connects the
   runController to. Handling of the received data is internal to the devices,
   which are under ITDM responsibility. The run controller limits application
   of new run configurations to the *ignore* state. Configurations pushed in
   other states will be ignored and an alarm will be raised on the run controller
   device, indicating that the configuration was not applied.


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

	scanDevice = connectDevice("SPB/GenericScan")
	scanDevice.start = 0
	scanDevice.stop = 10
	scanDevice.steps = 100

	#SimpleRunAcquisition is a wrapper to ITDM run management
	#providing a simplified interface for configuration. It also
	#triggers runs provides feedback to the scan devices calling it.
	scanAcquisition = connectDevice("SPB/SimpleRunAcquisition)
	sources = [ {"instance": "SASE1/SPB/SAMP/INJ_FLOW,", "behaviour": READONLY},
	            {"instance": "SASE1/SPB/SAMP/INJ_TEMP_1", "behaviour": RECORDALL},
	            {"instance": "SASE1/SPB/SAMP/INJ_TEMP_2", "behaviour": RECORDALL}]
	scanAcquisition.datasources = sources
	scanAcquisition.numTrains = 1000
	scanAcquisition.runConfigurator = "/SPB/RunConfigurator"
	scanAcquisition.runController = "/SASE1/RunCtrl"

	scanDevice.aquisitionConfig = "SPB/SimpleRunAcquisition"
	scanDevice.start()

.. figure:: images/gui_simple_run_config.png
   :alt: gui_simple_run_config.png

   The GUI interface of a simple run configuration device. The example
   here reflects the configuration resulting from the command line actions
   given in the code example. Clicking on *Trigger acquisition* will trigger
   the run controller to acquire data. Before, the run configurator will be
   update to reflect the sources configured in the simple acquisition device.


.. ifconfig:: includeDevInfo is True

	.. note::

	The implementation of the *SimpleRunAcquistion* device needs to take care that the
	previous run configuration can be restored by temporary storing of this config.

.. todo::

	Technically, it is possible to preserve all online streaming functionality by
	violating the principle that data on the stream outputs must be in the persisted
	file. One can discuss if for such a simple scan case we allow this.