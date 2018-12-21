.. _data_logging:

***************
The Data Logger
***************

Karabo offers continuous logging of all properties by usage of the *Data Logger* service.
This service is realized as a set of Karabo devices, one for each device being
logged, which archive any changes of a property to disk and allow for indexing
them either by trainId (if provided) or timestamp.

The logging devices are managed by a data logger manager device, which manages
logging device creation on a list of servers it holds. Load-balancing of the
logging servers is provided in a round-robin fashion, i.e. servers are
subsequently assigned to new logging devices.

Upon initialization the logging manager requests the current system topology
and on basis of this information initiates any logging devices needed. Afterwards,
it monitors whether new device instances appear or existing instances are shutdown.
For new instances it creates a new logging device if necessary. For device
instances which are shutdown it makes sure the logging device closes the log
file and then shuts the log device instance down.


Distinction from Data Acquisition
=================================

The data acquisition system provided by the IT and data management group (ITDM) is
responsible for main scientific data acquisition, organized on a per-run basis. It has
the requirement to provide for strict train and pulse id correlation between different
data sources and persisting of all configured and available data-sources is paramount.

This contrasts to the data loggers, which do not have these strict correlation
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

Data logging devices are hidden at user access levels lower than admin. Their
instance id is given by the device instance id they log, prefixed with "Datalogger-".


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

.. function:: getHistory(device, "2009-09-01", "2009-09-02").someProperty

    returns a list of tuples, which contain all changes of *someProperty*
    between the two given dates. The tuple contains four fields, the
    seconds since 1970-01-01 UTC, the train ID, a flag whether this is
    the last row in a set (typically, the device has been switched off
    afterwards), and the value of the property at that time.

    The dates of the timespan are parsed using
    :func:`dateutil.parser.parse`, allowing many ways to write the date.
    The most precise way is to write "2009-09-01T15:32:12 UTC", but you may
    omit any part, like "10:32", only giving the time, where we assume
    the current day.  Unless specified otherwise, your local timezone is
    assumed.

    Another parameter, *maxNumData*, may be given, which gives the maximum
    number of data points to be returned. It defaults to 10000. The returned
    data will be reduced appropriately to still span the full timespan."""


Logging Format
==============

Log files are created and updated by the logging devices. Specifically,
two files are created in a directory corresponding to the logged device's
device id, containing subdirectories *raw* and *idx* for the log files and
index files respectively.

The raw directory
    holds *archive* files, suffixed by the index of the file which contain
    configuration changes of a device in the row format

    ========= ========== ========= ======== ======== ======== ========== =====
    timestamp (ISO 8601) timestamp (karabo) train id property value type value
    ========= ========== ========= ======== ======== ======== ========== =====

    Additionally, each row is designated as pending to be logged into the
    archival index (LOGIN), or if it has already been validated (VALID).
    Logins are required if a file is reopened, after a device instance has
    reappeared.

    These files are in ASCII text format and all properties of a device are
    stored subsequently in a single table.

    Additionally, an entry of the appended entry's index is maintained in
    an *archive_index* file each time a new log file is created or an existing
    one is reopened.

    Finally, schema updates to the device are stored in a *archive_schema* file
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
