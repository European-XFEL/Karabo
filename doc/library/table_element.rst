..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

Table Element
=============

The most complex leaf element a Karabo device can have is the table element.
It is defined as a ``TABLE_ELEMENT`` and ``VectorHash`` in C++/bound and MDL, respectively.
In the following, its requirements and limitations are described.
First and foremost,

- **Table elements must always have a row schema describing them**


Table Schema Rules and Restrictions
-----------------------------------

A table row schema can only be composed of **leaf type** elements.
Furthermore, the following elements are supported:

- 'BOOL'
- 'INT8'
- 'UINT8'
- 'INT16'
- 'UINT16'
- 'INT32'
- 'UINT32'
- 'INT64'
- 'UINT64'
- 'FLOAT'
- 'DOUBLE'
- 'STRING'

- 'VECTOR_BOOL'
- 'VECTOR_INT8'
- 'VECTOR_UINT8'
- 'VECTOR_INT16'
- 'VECTOR_UINT16'
- 'VECTOR_INT32'
- 'VECTOR_UINT32'
- 'VECTOR_INT64'
- 'VECTOR_UINT64'
- 'VECTOR_DOUBLE'
- 'VECTOR_FLOAT'
- 'VECTOR_STRING'

The following leaf elements are **NOT** supported:

- 'VECTOR_HASH'
- 'CHAR'
- 'HASH'
- 'SCHEMA'
- 'NONE'
- 'BYTE_ARRAY'
- 'VECTOR_CHAR'
- 'COMPLEX_FLOAT'
- 'COMPLEX_DOUBLE'
- 'VECTOR_COMPLEX_DOUBLE'
- 'VECTOR_COMPLEX_FLOAT'

Node Elements are **NOT** supported:

- ChoiceOfNodes
- ListOfNodes
- Nodes

**READONLY/RECONFIGURABLE**: A table element is only validated against the
accessMode declaration.
The columns are not validated against their access mode declaration.

Attributes
----------

Table columns must specifiy their **defaultValue** attribute. A table column should not
come without a default value.


From Karabo 2.11 this is validated and sanitized in clients. In future versions this might throw an error.

The simple schema of a table element does **NOT** consider or **support** the attributes for **cells**:

- absoluteError, relativeError
- allowedStates
- assignment
- accessMode
- archivePolicy
- alarmHigh
- alarmLow
- warnHigh
- warnLow
- daqPolicy
- regex

They are not taken into account.

On a **best-effort** basis, the *Karabo GUI* accounts for

- cell accessMode: READONLY / RECONFIGURABLE

of table element cells. On the list of supported attributes are:

- displayedName
- description
- minInc, maxInc, minExc, maxExc
- minSize, maxSize
- options
- global accessMode (columns shall match the accessMode of the table)

The property settings (cells) are validated on reconfiguration on device side.


.. _library/schema_evolution:

Schema evolution
----------------

Schema evolution is particularly difficult for configuration management.

Consider a configuration of a device stored externally (e.g. by the configuration
manager or via the project manager) after being commissioned.

If the schema of the device evolves, for example:

- a property is added
- a property is removed,
- a property changes its minimum or maximum limits
- a property changes its type
- a property changes its default value

The externally stored configuration is potentially corrupted.

The *karaboGUI* will make a best effort attempt to sanitize the configuration by removing dead
properties, using defaults on new properties (if defined by the devices schema).
There is, however, no way to guarantee that the now-sanitized configuration is valid for the
new device and the device should be re-commissioned.

The schema evolution is especially difficult in the case of the table element.
While a device configuration can be partially defined (i.e. not all properties have to have a value),
in a table element property, all "cells" of each row **must** be defined to represent a valid table
property.

For these reasons, the ``rowSchema`` on a device will inject default values
if missing from the static element and the missing cells will be added.
