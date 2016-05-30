.. _howto-properties:

Properties of macros and devices
================================

One can define properties for both macros and devices. Those are variables
that are may be read or changed from the outside, e.g. with the GUI, or
from other devices. Variables which are not supposed to be visible from
the outside don't need to be declared, as always in python.

General attributes of properties
--------------------------------

All properties have a key, that is the name they have been declared with
in the class, as in `key = String()`. The others are:

`displayedName`
  should be a human-readable name. It is shown in the GUI. Keep it short.

`description`
  this is a human-readable description of the property. This is the place
  to write all the gory details.

`allowedStates`
  this are the states of a device where this property makes sense. This
  is especially important for slots: you can only trigger them if the
  device is in an allowed state. By default, all states are allowed.

`defaultValue`
  This is the default value if the property has not yet been set otherwise.

`accessMode`
  defines how this property can be accessed, from the
  :class:`~karabo.middlelayer_api.enums.AccessMode` enum. One of
  :const:`~karabo.middlelayer_api.enums.AccessMode.INITONLY`, in which case it
  can only be set upon initialization,
  :const:`~karabo.middlelayer_api.enums.AccessMode.READONLY`, if only the
  device can set this property, or
  :const:`~karabo.middlelayer_api.enums.AccessMode.RECONFIGURABLE`, if anyone
  may change the value of this property.

`assignment`
   this defines whether this property actually needs to have a value,
   :const:`~karabo.middlelayer_api.enums.Assignment.OPTIONAL` and
   :const:`~karabo.middlelayer_api.enums.Assignment.MANDATORY` are the options.

`requiredAccessLevel`
   the access level a user needs to have in order to see or modify this
   property.


Number Types
------------

Numbers are integers and float values. There are many
integer datatypes for the bit widths 8, 16, 32 and 64, both signed and
unsigned, as :class:`~karabo.middlelayer_api.hash.UInt32`. The two
floating-point types are :class:`~karabo.middlelayer_api.hash.Float` and
:class:`~karabo.middlelayer_api.hash.Double`

The numeric types have some more possible attributes:

`unitSymbol`
   gives the unit this property has. It is something like
   :const:`~karabo.middlelayer_api.enums.Unit.METER`.

`metricPrefixSymbol`
   gives the metric prefix of the above units. Something like
   :const:`~karabo.middlelayer_api.enums.MetricPrefix.KILO`.

`minExc`, `minInc`
   is the minimum value this property may have, either exclusive or inclusive.

`maxExc`, `maxInc`
   the same for the maximum
