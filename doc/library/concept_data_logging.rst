..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _data_logging:

***************
The Data Logger
***************

Karabo offers continuous logging of all properties by usage of the *Data Logger* service.
This service is realized as a set of Karabo devices, which archive any changes of a
property and allow for indexing by trainId (if provided) or timestamp.

The logging devices are managed by a data logger manager device, which manages
logging device creation on a list of servers it holds. Load-balancing of the
logging servers is provided in a round-robin fashion, i.e. servers are
subsequently assigned to new logging devices.

Upon initialization, the logging manager requests the current system topology
and based on this information initiates any logging devices needed. Afterwards,
it monitors whether new device instances appear or existing instances are shutdown.
For new instances it assigns the instance to a logging device. For device
instances which are shutdown it makes sure the logging device flushes all the logged
data.

The data logger originally used a text file based archiving backend. Then a new
backend, based on InFluxDB, has been integrated into the data logging infrastructure.
The run configuration file for the device server that hosts the DataLoggerManager
instance is where the definition and configuration of the archiving backend to use is
made. On a default Karabo installation, this run configuration file is located at
"$KARABO/var/service/karabo_dataLoggerManager". For reference, an example of a run
configuration file using the text file based backend is presented below:

.. code-block:: bash

   #!/bin/bash
   # This file is part of the initial configuration of Karabo
   cd $KARABO/var/data
   exec envdir $KARABO/var/environment karabo-cppserver serverId=karabo/dataLoggerManager \
   'deviceClasses=DataLoggerManager' \
   '{"KaraboDataLoggerManager" : {"classId": "DataLoggerManager", "serverList": "karabo/dataLogger"}}' \
   'Logger.priority=INFO' 2>&1


An example of a run configuration file using the InfluxDB based backend:

.. code-block:: bash

   #!/bin/bash
   # Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
   # This file is part of the initial configuration of Karabo
   cd $KARABO/var/data
   exec envdir $KARABO/var/environment karabo-cppserver serverId=karabo/dataLoggerManager \
   'deviceClasses=DataLoggerManager' \
   'init={"KaraboDataLoggerManager" : {"classId": "DataLoggerManager", "logger": "InfluxDataLogger", "influxDataLogger": {"urlRead": "tcp://localhost:8086", "urlWrite": "tcp://localhost:8086"}}}' \
   'Logger.priority=INFO' 2>&1

A default Karabo installation comes configured to use the text file based archiving backend.


Distinction from Data Acquisition
=================================

In XFEL, a data acquisition system provided by its IT and data management group (ITDM) is
responsible for main scientific data acquisition, organized on a per-run basis. It has
the requirement to provide for strict train and pulse id correlation between different
data sources and persisting of all configured and available data-sources is paramount.

This contrasts with the data loggers, which do not have those strict correlation
requirements: train ids are tagged and provided, but data sources providing
only time-stamps or much less regular updates to their values are much more common.
In terms of persisting data, the requirement is somewhat relaxed for each individual
data point, but a high uptime and availability are necessary.

Configuring Logging
===================

Logging is enabled on a per-device level by setting the archiving flag. In this case
all properties of the device will be logged, as will be the time of execution of
commands.

The data logger manager further allows to configure the device servers to use
of logging, by providing a list of server ids. Additionally, the maximum file
size of a single log file can be given, allowing for optimizing the indexing
performance, as closed off log files contain information on the timestamps of
the data they contain.

Data logging devices are hidden at user access levels lower than admin. The devices
being logged by each data logger instance can be found in its *devicesToBeLogged*
property.


Retrieving Logged Information
=============================

Logged information can be retrieved in multiple ways: through the command
line interface, iKarabo and the GUI. The first two ways are explicit calls
to the data logger readers to return information as specified by the
user. In the GUI case, information is implicitly retrieved when accessing
the history of trendlines. Accordingly, the first two use cases are documented
here.

In the device client the following functions exist for retrieving logger
information

.. function:: getPropertyHistory(deviceId, property, from, to, maxNumData)

    returns a vector of hashes of the ``property`` on device id ``from`` a given
    timestamp (ISO 8601 string) ``to`` a second. If ``to`` is an empty string
    it defaults to the current time. Finally, ``maxNumData`` limits the number
    of entries returned.

    Each entry in the returned vector of hashes has a property *v* of the
    correct value type of the archived property with the
    appropriate time stamp attributes set.

.. function:: getDataLogReader(deviceId)

    returns the device id of the data log reader associated to the device
    on ``deviceId``


.. function:: getConfigurationFromPast(deviceId, timepoint)

    returns a pair of the complete configuration and schema of ``deviceId`` at
    ``timepoint``, which is expected to be given as an ISO 8601 string.

In iKarabo the *getHistory* proxy object may be used:

.. function:: getHistory(device.someProperty, "2009-09-01", "2009-09-02")

    returns a list of tuples, which contain all changes of *someProperty*
    between the two given dates. The tuple contains four fields, the
    seconds since 1970-01-01 UTC, the train ID, a flag whether this is
    the last row in a set (typically, the device has been switched off
    afterwards), and the value of the property at that time.

    The second date is optional. If missing, the current time will be
    used as an end time.

    The dates of the timespan are parsed using
    :func:`dateutil.parser.parse`, allowing many ways to write the date.
    The most precise way is to write "2009-09-01T15:32:12 UTC", but you may
    omit any part, like "10:32", only giving the time, where we assume
    the current day.  Unless specified otherwise, your local timezone is
    assumed. See below for helper functions such as ``minutesAgo``.

    Another parameter, *maxNumData*, may be given, which gives the maximum
    number of data points to be returned. It defaults to 10000, the current
    maximum

.. function:: getHistory("device.someProperty", "2009-09-01", "2009-09-02")

    alternative implementation of ``getHistory`` without needing to create
    a proxy

.. function:: minutesAgo(n)

    returns a string containing the timepoint of ``n`` minutes ago in a format
    compatible with the expectations of ``getHistory``.

.. function:: hoursAgo(n)

    returns a string containing the timepoint of ``n`` hours ago in a format
    compatible with the expectations of ``getHistory``.

.. function:: daysAgo(n)

    returns a string containing the timepoint of ``n`` days ago in a format
    compatible with the expectations of ``getHistory``.

Text-File based Backend
=======================

Logging Format
--------------

Log files are created and updated by the logging devices. Specifically,
two files are created in a directory corresponding to the logged device's
device id, containing subdirectories *raw* and *idx* for the log files and
index files, respectively. The tree with the log files for the different devices
is rooted at $KARABO/var/data/karaboHistory.

The raw directory
    holds *archive_<n>.txt* files, where the suffix *n* is the index of the file which contains
    configuration changes of a device in the row format:

    ==================== ================== ======== ======== ========== =====
    timestamp (ISO 8601) timestamp (karabo) train id property value type value
    ==================== ================== ======== ======== ========== =====

    Additionally, each row is designated as pending to be logged into the
    archival index (LOGIN), or if it has already been validated (VALID).
    Logins are required if a file is reopened, after a device instance has
    reappeared.

    These files are in ASCII text format and all properties of a device are
    stored subsequently in a single table.

    An entry of the appended entry's index is maintained in an *archive_index.txt*
    file each time a new log file is created or an existing one is reopened.

    A file named *archive.last* stores the last index used as a suffix for naming the
    configuration changes files of the device.

    Finally, schema updates to the device are stored in a *archive_schema.txt* file
    while saves the XML serialized schema, alongside timestamp and train id
    information.

The idx directory
    holds *index* files, which are recorded for each property and hold the
    positions of the archival data for each property in the *raw* data files.
    These are binary files which store information on the timestamps of an entry
    its train id, it run and experiment number as well as the position in the
    *raw* data file.

Both index and raw files are regularly flushed to disk in the time interval
specified by the *flushInterval* property of the data logger.

InfluxDB based Backend
======================

Server infrastructure
---------------------

An instance of InfluxDB should be available when the karabo services are started.
A local instance of InfluxDB can be started by using the command **karabo-startinfluxdb**.

Logging Database Organization
-----------------------------

Each Karabo topic will have its own InfluxDB database. In each database, the
data will be organized in the set of measurements described below:

* **Device Properties Measurement**: Each device being logged in the topic will
  have its own measurement, with the name of the device. The device properties
  being logged will be mapped to fields with the same name as the property. The
  trainIds associated to the logging records will also be mapped to a field. The
  name of the user responsible for the property value change will be mapped to
  a tag in the device measurement. The value of the **karabo_user** tag will be
  either a user name (for changes associated to a user) or "." for changes that
  have no responsible user associated.

  An example of a device measurement - in this case for device 'GUI_SERVER_0':

  ==================== ============= ============= ================= ================== ======================
  Name: GUI_SERVER_0
  ------------------------------------------------------------------------------------------------------------
  time                 *karabo_user* _tid          serverId-STRING   useTimeServer-BOOL connectedClients-INT32
  ==================== ============= ============= ================= ================== ======================
  2019-10-24T10:54:04Z .             0             karabo/gui_server True               10
  2019-10-24T10:56:28Z Alice         1272                            False
  2019-10-24T11:00:02Z Bob           0                                                  9
  ==================== ============= ============= ================= ================== ======================

  As shown in the example, the number of non-null fields varies among records -
  the data logger will group the properties by the time they changed before writing
  them to InfluxDB. The timestamps for **time** are explicitly specified when data is
  sent to InfluxDB. **karabo_user** is a tag. All the other columns are fields. Field names
  are mangled in order to support schema evolution. The mangling consists of adding
  the suffix "-[KARABO_TYPE]" to the field name. Properties with
  redundant values, like **_device_id_** and **deviceId**, shouldn't be logged.

* **Device Events Measurement**: This measurement will store the device events - currently
  device instantiations, shutdowns and schema updates.

  The log reader relies on device instantiation events for being able to retrieve the last
  known configuration if the given time point is not in an interval during which the device
  was active. Similarly, **DeviceClient.getPropertyHistory** relies on instatiantion events
  to know from when it must start its properties read sweep in case there is no change for
  the given property during the requested time interval.

  An example of a device events measurement - for device 'GUI_SERVER_0':

  ==================== ====== ============== =================
  Name: GUI_SERVER_0__EVENTS
  ------------------------------------------------------------
  time                 *type* schema_digest  karabo_user
  ==================== ====== ============== =================
  2019-10-24T10:54:04Z +LOG                  Bob
  2019-10-24T10:56:28Z SCHEMA 3fd545689a12ce .
  2019-10-24T11:00:02Z -LOG                  Alice
  ==================== ====== ============== =================

  The timestamps for time are explicitly specified when data is sent to InfluxDB. **type**
  is a tag whose value indicates the type of the event. The remaining columns are fields.
  **schema_digest** is a digest for a serialized schema stored in the Device Schema
  Measurement described in the next item. **karabo_user** is the athenticated user that either
  instantiated or shutdown the device (not active yet - for now, it will always be "**.**").

* **Device Schema Measurement**:

  ==================== =============== ============ =========== ==================================================
  Name: GUI_SERVER_0__SCHEMAS
  ----------------------------------------------------------------------------------------------------------------
  time                 *digest*        digest_start schema_size schema
  ==================== =============== ============ =========== ==================================================
  2019-10-24T10:54:04Z 3fd545689a12ce  3fd54567     5349        RGF0YUdlbmVyYXRvcjo8P3htbCB2ZXJRGF0YUdlyYXRvcj ...
  ==================== =============== ============ =========== ==================================================

  The **schema** saved in the database is the base64 enconding of the device's schema as serialized
  in binary form by the Karabo Framework. The **digest** is the SHA-1 hash of the binary serialized
  form of the schema (before it is encoded in base64).
  The **digest_start** and **schema_size** fields exist to ease exploration of data
  stored in the Device Schema measurement:  InfluxQL only allows tag values to be output in the
  results of a query if there's at least one field in the query selection. If **schema** was the only
  field in the measurement, with its usually huge string values, any attempt to list digests in query
  results would be cumbersome as the full schema values would also have to be output. To add to that
  limitation, InfluxQL also lacks any function that allows to return either the length or a part of
  a string metric.

For the production environment, the replication factors of the retention policies
described above match the number of InfluxDB servers in the cluster. The durations of
the retention policies should be the same for all the measurements. The exact durations
have yet to be defined.
