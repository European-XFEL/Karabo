*************
 Data Logging
*************

Data Logging is a service consisting of several components (devices):  DataLoggerManager, DataLogger devices and
DataLogReader.  They may run on the same device server or may be spread across separate device servers that
may run on separate hosts for performance reasons. 

DataLoggerManager
=================

The DataLoggerManager device is subscribed to the broker messages to receive notifications about appearing the 
new device instance or if some device instance is gone.  If the new device instance (user device) is detected
the DataLoggerManager analyses the topology object of the device to check if the device should be archived and
if yes it creates a DataLogger instance for archiving the user device properties. So each user device has 
corresponding DataLogger device if needed.

If user device is gone, the DataLoggerManager closes all active streams on DataLogger device (via DeviceClient API) and 
shuts it down.

NOTE: DataLogger devices can be started on different device servers and hosts. It can be configured.

DataLogger
==========

DataLogger device is subscribed to "signalChanged" signal of the served user device.  As a result the "slotChanged"
callback is called with the new configuration parameters  (Hash).  The DataLogger writes several output streams:
see below. Some of them are located in "raw" directory, and the others in "idx" directory.

Configuration stream
--------------------

It is a text stream with each line describing one property (configuration parameter) with 10 fields:

* Timestamp in extended ISO8601 combined date and time format, like 20150723T093858.291366Z
* Timestamp as double value, like 1437644338.291366
* Timestamp part in seconds, like 1437644338
* Timestamp part in attosecs, like 291366730000000000
* TrainId as unsigned 64-bits long number
* Property name
* Property type (karabo reference type), like BOOL or VECTOR_INT16
* Value as a string representation, like 1 or 7452,4788,21582,32382,30427
* User, like "operator" or "Jones"
* Flag, like VALID, LOGIN, LOGOUT etc  (may be extended in the future)

The size of such a file is configurable (default is 100MB). The name of the file is build using
a few fields separated by "_" (undescore).

* deviceId, like "gen" or "exflserv14043_Karabo_CppServer_Conveyor_123"
* the word "configuration"
* file number.  It is just sequential monotonic integer number.
* file extension "txt"

Example: gen_configuration_8.txt

Content stream
--------------

This is a file that can be built using "configuration stream". The file is <deviceId>_index.txt.

Example: gen_index.txt

Each record represents some event and has the following structure:

* Event (flag), like +LOG (LOGIN), -LOG (LOGOUT), =NEW ("next" file with incremented file number).  It may be extended
* ISO8601 timestamp
* Timestamp as double
* Timestamp (seconds)
* Timestamp (attoseconds)
* TrainId
* File position in "configuration stream".
* User
* File number

The content file is inspected for fast approximate navigation using timestamp, namely, for getting exact file number.

Last number file
----------------

This file (<deviceId>.last) stores the last file number used.

Schema.
-------

This file (<deviceId>_schema.txt) contains records of timestamps (seconds, fraction), trainId and user device schema
 as XML string.

Binary index streams
--------------------

Binary index streams are considered as a solution to the performance problems that the people exprienced in
real applications due to growing set of archived configuration log files, especially regarding access time to the
data in DataLogReader.

As a possible solution, we consider building binary index files for each property that are created in sync with
"configuration files".  The file naming convention is the following ...

<deviceId>_configuration_<filenumber>-<property>-index.bin

Example: gen_configuration_5-integerProperty-index.bin

Such indexing applies only to "read-only" parameters and to those that were changed at least twice.

The record format of binary index file contains 5 fields:

     +-------------------------------------+----------+
     |   Field                             |  Length  |
     +=====================================+==========+
     | Timestamp as *Double*               |  8 bytes |
     +-------------------------------------+----------+
     | TrainId                             |  8 bytes |
     +-------------------------------------+----------+
     | File position in configuration file |  8 bytes |
     +-------------------------------------+----------+
     | Extent1                             |  4 bytes |
     +-------------------------------------+----------+
     | Extent2                             |  4 bytes |
     +-------------------------------------+----------+

Fixed record size (32 bytes) allows easy calculation at positioning in the index file and
applying selection of data points without touching the "raw" configuration files. See DataLogReader below.

Index tools
-----------
Some useful tools are located in $KARABO/bin directory
The binary index files can be inspected if needed by **idxview**:

    idxview <binary-index-file>

The index files may be rebuild or re-created offline using **idxbuild** command:
    
    idxbuild <top-karabo-history-directory>

Example:

    cd .../karaboRun/servers/dataLoggerServers

    idxbuild karaboHistory

Rebuilding reguires **raw** directory with "raw" configuration files ( <deviceId>_configuration_<number>.txt ),
schema history file ( <deviceId>_schema.txt ).  The "content" files ( <deviceId>_index.txt ) will be rebuild, the old one
will be renamed.  The old **idx** directory will be overwritten if it exists, otherwise the new one will be created.


 
DataLogReader
=============

DataLogReader device implements "slotGetPropertyHistory" slot function that is called by GuiServer device
on behalf of Gui client, for example, due to trendline widget requests. The request comes with information
about deviceId of user device, requested property and time range (from/to).   The DataLogReader does the following

* define file numbers for the "from" and "to" time request using content file
* find positions in property index file of "from" and "to" timestamps and get the structure containing index file record number and file number corresponding "from" timestamp, record number and file number corresponding the "to" timestamp and vector of number of entries in files that belongs to requested time range.
* calculate total number of entries containing in this vector (equal to sum of all vector element values)
* calculate "reduction factor" using requested number of data points.
* read "configuration files" via reduced binary index ( ~ 800 data points )

DataLogReader device is used by GuiServer device for executing requests about property
values in requested time range. 








 


