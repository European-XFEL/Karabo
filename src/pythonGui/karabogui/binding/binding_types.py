# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import numpy as np
from traits.api import (
    Array, CArray, Complex, Dict, Either, Enum, Event, HasStrictTraits,
    Instance, List, Property, String, Trait, TraitError, TraitHandler,
    Undefined, cached_property)

from karabo.common import const
from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, Assignment, Hash, HashList, Timestamp)

from .trait_types import CBool, Float, NumpyRange

KEY_DISPLAYED_NAME = const.KARABO_SCHEMA_DISPLAYED_NAME
KEY_DISPLAY_TYPE = const.KARABO_SCHEMA_DISPLAY_TYPE
KEY_ACCMODE = const.KARABO_SCHEMA_ACCESS_MODE
KEY_ASSIGNMENT = const.KARABO_SCHEMA_ASSIGNMENT
KEY_ACCLVL = const.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL
KEY_OPT = const.KARABO_SCHEMA_OPTIONS
KEY_UNIT = const.KARABO_SCHEMA_UNIT_SYMBOL
KEY_UNITPREFIX = const.KARABO_SCHEMA_METRIC_PREFIX_SYMBOL
KEY_ROW_SCHEMA = const.KARABO_SCHEMA_ROW_SCHEMA


class BaseBinding(HasStrictTraits):
    """The base class for all bindings. It represents a single node in an
    object created from a schema. It has a dictionary of attributes and a
    `value` trait which contains the value of the node. The value is validated
    using normal ``Traits`` validation.

    :param attributes: The dictionary of the attributes belonging to the
                       `BaseBinding`.
    :param config_update: ``Traits.Event`` which fires when the value is
                          updated externally
    :param historic_data: ``Traits.Event`` which fires when historic data
                          arrives for this object node. The data is contained
                          in the new value passed to notification handlers
    :param value: The value contained in this node. Derived classes should
                  redefine this. The default is ``Traits.Undefined``.
    :param timestamp: The timestamp when the value was last set on the device.
                      Instance of `karabo.native.Timestamp`.


    Attribute shortcuts for often used properties
    ---------------------------------------------

    :param displayedName: The displayed name of the property
    :param displayType: The display type of the property. This might provide
                         more options later on.
    :param accessMode: The access mode specification of the binding taking an
                        enum of `karabo.native.AccessMode`.
    :param assignment: The assignment setting with an enum of
                       `karabo.native.Assignment`.
    :param options: The list of options for this base binding.
    :param requiredAccessLevel: The required access level with enums of
                                  `karabo.native.AccessLevel`.
    :param unit_label: The unit label string combining the unit and metric
                       prefix.
    """
    # Attributes property copied from the object schema, we control the
    # setter of the attributes so their shortcuts can be updated in time
    # but avoid hammering the configurator

    attributes = Property
    _attributes = Dict
    # When the value was last set on the device
    timestamp = Instance(Timestamp)
    value = Undefined
    config_update = Event
    historic_data = Event

    # Attribute shortcuts
    displayedName = String
    displayType = String
    accessMode = Enum(*AccessMode)
    assignment = Enum(*Assignment)
    options = List
    requiredAccessLevel = Enum(*AccessLevel)
    unit_label = String

    def is_allowed(self, state):
        """Return True if the given `state` is an allowed state for this
        binding.

        :param state: Ideally an instance of `karabo.native.State` otherwise
                      a string that can be casted to a ``State`` enum.
        """
        if isinstance(state, State):
            state = state.value

        alloweds = self._attributes.get(const.KARABO_SCHEMA_ALLOWED_STATES, [])
        return alloweds == [] or state in alloweds

    def _get_attributes(self):
        return self._attributes

    def _set_attributes(self, value):
        self._attributes = value

    def update_attributes(self, attrs):
        """Silently update attributes dictionary"""
        if attrs:
            traits = {'_attributes': self._attributes}
            traits['_attributes'].update(attrs)
            self.trait_setq(**traits)
            self._update_shortcuts(attrs)

    def _displayedName_default(self):
        return self._attributes.get(KEY_DISPLAYED_NAME, '')

    def _displayType_default(self):
        return self._attributes.get(KEY_DISPLAY_TYPE, '')

    def _accessMode_default(self):
        mode = self._attributes.get(KEY_ACCMODE)
        return AccessMode.UNDEFINED if mode is None else AccessMode(mode)

    def _assignment_default(self):
        assign = self._attributes.get(KEY_ASSIGNMENT)
        return Assignment.OPTIONAL if assign is None else Assignment(assign)

    def _options_default(self):
        return self._attributes.get(KEY_OPT, [])

    def _requiredAccessLevel_default(self):
        level = self._attributes.get(KEY_ACCLVL)
        return AccessLevel.OBSERVER if level is None else AccessLevel(level)

    def _unit_label_default(self):
        attrs = self._attributes
        unit = attrs.get(KEY_UNITPREFIX, '') + attrs.get(KEY_UNIT, '')
        return unit

    def _update_shortcuts(self, attrs):
        if KEY_DISPLAYED_NAME in attrs:
            self.displayedName = attrs[KEY_DISPLAYED_NAME]
        if KEY_DISPLAY_TYPE in attrs:
            self.displayType = attrs[KEY_DISPLAY_TYPE]
        if KEY_ACCMODE in attrs:
            self.accessMode = AccessMode(attrs[KEY_ACCMODE])
        if KEY_ACCLVL in attrs:
            self.requiredAccessLevel = AccessLevel(attrs[KEY_ACCLVL])
        if KEY_ASSIGNMENT in attrs:
            self.assignment = Assignment(attrs[KEY_ASSIGNMENT])
        if KEY_OPT in attrs:
            self.options = attrs[KEY_OPT]
        if KEY_UNIT in attrs or KEY_UNITPREFIX in attrs:
            unit = attrs.get(KEY_UNITPREFIX, '') + attrs.get(KEY_UNIT, '')
            self.unit_label = unit

    def check(self, value):
        """Check and validate the value of this binding"""
        options = self._attributes.get(KEY_OPT, None)
        if options is not None and value not in options:
            raise TraitError(f"The value {value} is not in the allowed options"
                             f"{options}")

        return self.validate_trait("value", value)


class BindingNamespace:
    """A namespace which is iterable and remembers the order in which values
    were added. Additionally, the type of the values in the namespace can be
    restricted with an `item_type` which is a type or tuple of types.
    """

    def __init__(self, *, item_type=object):
        priv_prefix = '_' + type(self).__name__
        # Avoid __setattr__ recursion by modifying __dict__ directly
        self.__dict__[priv_prefix + '__type'] = item_type
        self.__dict__[priv_prefix + '__names'] = []
        self.__dict__[priv_prefix + '__priv_prefix'] = priv_prefix

    def clear_namespace(self):
        """Remove all items from the namespace"""
        func = super().__delattr__
        for key in self.__names:
            func(key)
        self.__names.clear()

    def __contains__(self, key):
        return key in self.__names

    def __delattr__(self, key):
        if not key.startswith(self.__priv_prefix):
            super().__delattr__(key)
            self.__names.remove(key)

    def __setattr__(self, key, value):
        if not isinstance(value, self.__type):
            msg = 'Attributes of type {} are expected, but type {} was passed'
            raise ValueError(msg.format(self.__type, type(value)))

        super().__setattr__(key, value)
        if key not in self.__names and not key.startswith(self.__priv_prefix):
            self.__names.append(key)

    def __iter__(self):
        return iter(self.__names)

    def __len__(self):
        return len(self.__names)

    def __repr__(self):
        names = '\n'.join('\t' + n + ': ' + repr(getattr(self, n))
                          for n in self)
        return f'{{\n{names}\n}}'


class BindingRoot(BaseBinding):
    """The root node of an object binding which was created from a schema.
    """
    # The top-level namespace of the object
    value = Instance(BindingNamespace, kw={'item_type': BaseBinding})
    # The name of the class represented
    class_id = String
    # An event which fires when the schema changes
    schema_update = Event
    # A cache for visible children names {accesslevel: [names]}
    # All bindings with their value type as BindingNamespace should have
    # this trait to cache the names in the value according to access level
    children_names = Dict()

    def _schema_update_fired(self):
        # when schema changes, clear cached children names
        self.children_names = {}


# =============================================================================
# Binding classes for each type expected in Karabo (see Schema.hh and Types.hh)
#

class BoolBinding(BaseBinding):
    value = CBool
    # number type binding (Bool, Float and Int) could have options in type of
    # numpy array, CArray will cast list to a numpy array. Be aware that when
    # comparing numpy arrays, result is an array as well.
    options = CArray

    def _options_default(self):
        return self._attributes.get(KEY_OPT, [])


class _ByteArrayHandler(TraitHandler):
    """A trait handler to work around middlelater stupidity.
    """

    def validate(self, obj, name, value):
        if isinstance(value, bytearray):
            return value
        elif isinstance(value, bytes):
            return bytearray(value)
        self.error(obj, name, value)


class ByteArrayBinding(BaseBinding):
    value = Trait(_ByteArrayHandler())


class CharBinding(BaseBinding):
    value = String(maxlen=1)


class ComplexBinding(BaseBinding):
    value = Complex


class FloatBinding(BaseBinding):
    value = Float
    options = CArray

    def _options_default(self):
        return self._attributes.get(KEY_OPT, [])

    def check(self, value):
        value = super().check(value)
        # Karabo attribute check
        low, high = self.getMinMax()
        if value < low:
            raise TraitError(f"The value {value} is lower than {low}")
        if value > high:
            raise TraitError(f"The value {value} is higher than {high}")

        return value

    def getMinMax(self):
        attrs = self._attributes
        value_type = attrs.get(const.KARABO_SCHEMA_VALUE_TYPE)
        if value_type in ('FLOAT', 'COMPLEX_FLOAT'):
            info = np.finfo(np.float32)
        else:
            info = np.finfo(np.float64)

        low = attrs.get(const.KARABO_SCHEMA_MIN_EXC)
        if low is not None:
            low = low * (1 + np.sign(low) * info.eps) + info.tiny
        else:
            low = attrs.get(const.KARABO_SCHEMA_MIN_INC, info.min)

        high = attrs.get(const.KARABO_SCHEMA_MAX_EXC)
        if high is not None:
            high = high * (1 - np.sign(high) * info.eps) - info.tiny
        else:
            high = attrs.get(const.KARABO_SCHEMA_MAX_INC, info.max)

        return low, high


class HashBinding(BaseBinding):
    value = Instance(Hash)


class IntBinding(BaseBinding):
    """The base class for all integer binding types"""
    options = CArray

    def _options_default(self):
        return self._attributes.get(KEY_OPT, [])

    def check(self, value):
        value = super().check(value)
        # Karabo attribute check
        low, high = self.getMinMax()
        if value < low:
            raise TraitError(f"The value {value} is lower than {low}")
        if value > high:
            raise TraitError(f"The value {value} is higher than {high}")

        return value

    def getMinMax(self):
        range_trait = self.trait('value').handler
        value_range = range_trait._low, range_trait._high
        if range_trait._exclude_low:
            value_range = (value_range[0] + 1, value_range[1])
        if range_trait._exclude_high:
            value_range = (value_range[0], value_range[1] - 1)

        attrs = self._attributes
        low = attrs.get(const.KARABO_SCHEMA_MIN_EXC)
        if low is not None:
            low += 1
        else:
            low = attrs.get(const.KARABO_SCHEMA_MIN_INC, value_range[0])

        high = attrs.get(const.KARABO_SCHEMA_MAX_EXC)
        if high is not None:
            high -= 1
        else:
            high = attrs.get(const.KARABO_SCHEMA_MAX_INC, value_range[1])

        return low, high


class SignedIntBinding(IntBinding):
    """The base class for all signed integer binding types"""


class UnsignedIntBinding(IntBinding):
    """The base class for all unsigned integer binding types"""


class Int8Binding(SignedIntBinding):
    value = NumpyRange(low=(-1 - (1 << 7)), high=(1 << 7), value=0,
                       exclude_low=True, exclude_high=True, dtype=np.int8)


class Int16Binding(SignedIntBinding):
    value = NumpyRange(low=(-1 - (1 << 15)), high=(1 << 15), value=0,
                       exclude_low=True, exclude_high=True, dtype=np.int16)


class Int32Binding(SignedIntBinding):
    value = NumpyRange(low=(-1 - (1 << 31)), high=(1 << 31), value=0,
                       exclude_low=True, exclude_high=True, dtype=np.int32)


class Int64Binding(SignedIntBinding):
    value = NumpyRange(low=(-1 - (1 << 63)), high=(1 << 63), value=0,
                       exclude_low=True, exclude_high=True, dtype=np.int64)


class NodeBinding(BaseBinding):
    value = Instance(BindingNamespace)
    # Visible children names {accesslevel: [names]}
    children_names = Dict()


class ImageBinding(NodeBinding):
    pass  # Nothing to add. We just need a different class for Image


class NDArrayBinding(NodeBinding):
    pass  # Nothing to add. We just need a different class for NDArray


class WidgetNodeBinding(NodeBinding):
    pass  # Nothing to add. We just need a different class for WidgetNode


class NoneBinding(BaseBinding):
    value = Undefined


class PipelineOutputBinding(NodeBinding):
    pass  # Nothing to add. We just need a different class for PipelineOutput


class SchemaBinding(BaseBinding):
    value = Instance(object)


class SlotBinding(NodeBinding):
    pass  # Nothing to add. We just need a different class for Slots


class StringBinding(BaseBinding):
    value = String


class Uint8Binding(UnsignedIntBinding):
    value = NumpyRange(low=0, high=(1 << 8), value=0, exclude_high=True,
                       dtype=np.uint8)


class Uint16Binding(UnsignedIntBinding):
    value = NumpyRange(low=0, high=(1 << 16), value=0, exclude_high=True,
                       dtype=np.uint16)


class Uint32Binding(UnsignedIntBinding):
    value = NumpyRange(low=0, high=(1 << 32), value=0, exclude_high=True,
                       dtype=np.uint32)


class Uint64Binding(UnsignedIntBinding):
    value = NumpyRange(low=0, high=(1 << 64), value=0, exclude_high=True,
                       dtype=np.uint64)


class VectorBinding(BaseBinding):
    """The base class for all vector binding types"""

    def check(self, value):
        value = super().check(value)
        # Karabo attribute check for minSize and maxSize
        attributes = self._attributes
        minSize = attributes.get(const.KARABO_SCHEMA_MIN_SIZE)
        if minSize is not None and len(value) < minSize:
            raise TraitError(f"Vector with size {len(value)} is shorter than "
                             f"the allowed size of {minSize}")
        maxSize = attributes.get(const.KARABO_SCHEMA_MAX_SIZE)
        if maxSize is not None and len(value) > maxSize:
            raise TraitError(f"Vector with size {len(value)} is larger than "
                             f"the allowed size of {maxSize}")
        return value


class VectorNumberBinding(VectorBinding):
    """The base class for all vector binding types which contain types
    supported by numpy.
    """


class VectorBoolBinding(VectorNumberBinding):
    value = Array(dtype='bool')


class _ByteHandler(TraitHandler):
    """A trait handler to handle string to bytes conversion
    """

    def validate(self, obj, name, value):
        if isinstance(value, bytes):
            return value
        elif isinstance(value, str):
            return value.encode()
        self.error(obj, name, value)


class VectorCharBinding(VectorBinding):
    value = Trait(_ByteHandler())


class VectorComplexDoubleBinding(VectorNumberBinding):
    value = Array(dtype='complex128', shape=(None,))


class VectorComplexFloatBinding(VectorNumberBinding):
    value = Array(dtype='complex64', shape=(None,))


class VectorDoubleBinding(VectorNumberBinding):
    value = Array(dtype='float64', shape=(None,))


class VectorFloatBinding(VectorNumberBinding):
    value = Array(dtype='float32', shape=(None,))


class VectorHashBinding(VectorBinding):
    """The VectorHash Binding for Table elements of devices

    :param row_schema: Instance of Hash containing the row schema
    :param bindings: A dictionary of bindings associated to the column keys
    """
    value = Either((Instance(HashList), List))
    row_schema = Instance(Hash)
    bindings = Property(depends_on="row_schema")

    @cached_property
    def _get_bindings(self):
        from .builder import _BINDING_MAP
        bindings = {}
        for key in self.row_schema.getKeys():
            attrs = self.row_schema[key, ...]
            value_type = attrs[const.KARABO_SCHEMA_VALUE_TYPE]
            binding_factory = _BINDING_MAP[value_type]
            bindings[key] = binding_factory(attributes=attrs, value=Undefined)
        return bindings

    def get_binding(self, key):
        return self.bindings.get(key)

    def _row_schema_default(self):
        row_schema = self._attributes.get(KEY_ROW_SCHEMA)
        if row_schema is None:
            return Hash()

        return row_schema.hash

    def _update_shortcuts(self, attrs):
        super()._update_shortcuts(attrs)
        row_schema = attrs.get(KEY_ROW_SCHEMA)
        if row_schema is None:
            return Hash()

        self.row_schema = row_schema.hash


class VectorInt8Binding(VectorNumberBinding):
    value = Array(dtype='int8', shape=(None,))


class VectorInt16Binding(VectorNumberBinding):
    value = Array(dtype='int16', shape=(None,))


class VectorInt32Binding(VectorNumberBinding):
    value = Array(dtype='int32', shape=(None,))


class VectorInt64Binding(VectorNumberBinding):
    value = Array(dtype='int64', shape=(None,))


class VectorNoneBinding(VectorBinding):
    value = List(Undefined)


class VectorStringBinding(VectorBinding):
    value = List(String)


class VectorUint8Binding(VectorNumberBinding):
    value = Array(dtype='uint8', shape=(None,))


class VectorUint16Binding(VectorNumberBinding):
    value = Array(dtype='uint16', shape=(None,))


class VectorUint32Binding(VectorNumberBinding):
    value = Array(dtype='uint32', shape=(None,))


class VectorUint64Binding(VectorNumberBinding):
    value = Array(dtype='uint64', shape=(None,))


class TableBinding(VectorHashBinding):
    pass  # Nothing to add. We just need a different class for Tables
