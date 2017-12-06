from traits.api import (
    HasStrictTraits, Array, Bool, Bytes, Complex, Dict, Event, Float, Instance,
    List, Property, Range, String, Trait, TraitHandler, Undefined
)

from karabo.middlelayer import (AccessLevel, AccessMode, Assignment, Hash,
                                State, Timestamp)
from . import const


class BaseBinding(HasStrictTraits):
    """The base class for all bindings. It represents a single node in an
    object created from a schema. It has a dictionary of attributes and a
    `value` trait which contains the value of the node. The value is validated
    using normal Traits validation.
    """
    # Dictionary of attributes copied from the object schema
    attributes = Dict
    # When the value was last set on the device
    timestamp = Instance(Timestamp)
    # The value contained in this node. Derived classes should redefine this.
    value = Undefined

    # An event which fires when the value is updated externally
    config_update = Event
    # A event which fires when historic data arrives for this object node
    # The data is contained in the new value passed to notification handlers
    historic_data = Event

    # Attribute shortcut properties
    access_mode = Property
    assignment = Property
    options = Property
    required_access_level = Property
    unit_label = Property

    def is_allowed(self, state):
        """Return True if the given `state` is an allowed state for this
        binding.
        """
        if isinstance(state, State):
            state = state.value

        alloweds = self.attributes.get(const.KARABO_SCHEMA_ALLOWED_STATES, [])
        return alloweds == [] or state in alloweds

    def _get_access_mode(self):
        mode = self.attributes.get(const.KARABO_SCHEMA_ACCESS_MODE)
        return AccessMode.UNDEFINED if mode is None else AccessMode(mode)

    def _get_assignment(self):
        assign = self.attributes.get(const.KARABO_SCHEMA_ASSIGNMENT)
        return Assignment.OPTIONAL if assign is None else Assignment(assign)

    def _get_options(self):
        return self.attributes.get(const.KARABO_SCHEMA_OPTIONS, [])

    def _get_required_access_level(self):
        level = self.attributes.get(const.KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)
        return AccessLevel.OBSERVER if level is None else AccessLevel(level)

    def _get_unit_label(self):
        attrs = self.attributes
        prefix_symbol = attrs.get(const.KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, '')
        unit_symbol = attrs.get(const.KARABO_SCHEMA_UNIT_SYMBOL, '')
        return prefix_symbol + unit_symbol


class BindingNamespace(object):
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

    def __contains__(self, key):
        return key in self.__names

    def __delattr__(self, key):
        if not key.startswith(self.__priv_prefix):
            super(BindingNamespace, self).__delattr__(key)
            self.__names.remove(key)

    def __setattr__(self, key, value):
        if not isinstance(value, self.__type):
            msg = 'Attributes of type {} are expected, but type {} was passed'
            raise ValueError(msg.format(self.__type, type(value)))

        super(BindingNamespace, self).__setattr__(key, value)
        if key not in self.__names and not key.startswith(self.__priv_prefix):
            self.__names.append(key)

    def __iter__(self):
        return iter(self.__names)

    def __len__(self):
        return len(self.__names)

    def __repr__(self):
        names = '\n'.join('\t' + n + ': ' + repr(getattr(self, n))
                          for n in self)
        return '{{\n{}\n}}'.format(names)


class BindingRoot(HasStrictTraits):
    """The root node of an object binding which was created from a schema.
    """
    # The name of the class represented
    class_id = String
    # The top-level namespace of the object
    value = Instance(BindingNamespace, kw={'item_type': BaseBinding})
    # An event which fires when the schema changes
    schema_update = Event
    # An event which fires when subnodes/leaves are updated
    config_update = Event


# =============================================================================
# Binding classes for each type expected in Karabo (see Schema.hh and Types.hh)
#

class BoolBinding(BaseBinding):
    value = Bool


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


class HashBinding(BaseBinding):
    value = Instance(Hash)


class IntBinding(BaseBinding):
    """The base class for all integer binding types"""


class SignedIntBinding(IntBinding):
    """The base class for all signed integer binding types"""


class UnsignedIntBinding(IntBinding):
    """The base class for all unsigned integer binding types"""


class Int8Binding(SignedIntBinding):
    value = Range(low=-(1 << 7), high=(1 << 7), value=0,
                  exclude_low=True, exclude_high=True)


class Int16Binding(SignedIntBinding):
    value = Range(low=-(1 << 15), high=(1 << 15), value=0,
                  exclude_low=True, exclude_high=True)


class Int32Binding(SignedIntBinding):
    value = Range(low=-(1 << 31), high=(1 << 31), value=0,
                  exclude_low=True, exclude_high=True)


class Int64Binding(SignedIntBinding):
    value = Range(low=-(1 << 63), high=(1 << 63), value=0,
                  exclude_low=True, exclude_high=True)


class NodeBinding(BaseBinding):
    value = Instance(BindingNamespace)


class ImageBinding(NodeBinding):
    pass  # Nothing to add. We just need a different class for Image


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
    value = Range(low=0, high=(1 << 8), value=0, exclude_high=True)


class Uint16Binding(UnsignedIntBinding):
    value = Range(low=0, high=(1 << 16), value=0, exclude_high=True)


class Uint32Binding(UnsignedIntBinding):
    value = Range(low=0, high=(1 << 32), value=0, exclude_high=True)


class Uint64Binding(UnsignedIntBinding):
    value = Range(low=0, high=(1 << 64), value=0, exclude_high=True)


class VectorBinding(BaseBinding):
    """The base class for all vector binding types"""


class VectorBoolBinding(VectorBinding):
    value = Array(dtype='bool')


class VectorCharBinding(VectorBinding):
    value = Bytes


class VectorComplexDoubleBinding(VectorBinding):
    value = Array(dtype='complex128', shape=(None,))


class VectorComplexFloatBinding(VectorBinding):
    value = Array(dtype='complex64', shape=(None,))


class VectorDoubleBinding(VectorBinding):
    value = Array(dtype='float64', shape=(None,))


class VectorFloatBinding(VectorBinding):
    value = Array(dtype='float32', shape=(None,))


class VectorHashBinding(VectorBinding):
    value = List(Instance(Hash))


class VectorInt8Binding(VectorBinding):
    value = Array(dtype='int8', shape=(None,))


class VectorInt16Binding(VectorBinding):
    value = Array(dtype='int16', shape=(None,))


class VectorInt32Binding(VectorBinding):
    value = Array(dtype='int32', shape=(None,))


class VectorInt64Binding(VectorBinding):
    value = Array(dtype='int64', shape=(None,))


class VectorNoneBinding(VectorBinding):
    value = List(Undefined)


class VectorStringBinding(VectorBinding):
    value = List(String)


class VectorUint8Binding(VectorBinding):
    value = Array(dtype='uint8', shape=(None,))


class VectorUint16Binding(VectorBinding):
    value = Array(dtype='uint16', shape=(None,))


class VectorUint32Binding(VectorBinding):
    value = Array(dtype='uint32', shape=(None,))


class VectorUint64Binding(VectorBinding):
    value = Array(dtype='uint64', shape=(None,))


class TableBinding(VectorHashBinding):
    pass  # Nothing to add. We just need a different class for Tables
