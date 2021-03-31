import numpy as np
from traits.api import Undefined

from karabo.common import const
from karabo.common.api import (
    KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_EXC,
    KARABO_SCHEMA_MIN_INC)
from . import types


def get_editor_value(property_proxy, default=None):
    """Return the correct value of a PropertyProxy to show in an editor

    :param default: return default if value is None or Undefined
    """
    value = property_proxy.edit_value
    if value is None or value is Undefined:
        return get_binding_value(property_proxy, default)
    return value


def get_binding_value(binding, default=None):
    """Get the binding value, this function is used to deal with Undefined or
    None binding values.

    :param default: return default if value is None or Undefined
    """
    value = binding.value
    return default if value is None or value is Undefined else value


def get_min_max(binding):
    """Given a BaseBinding instance, return the minimum and maximum values
    which can be assigned to its `value` trait.
    """
    INT_TYPES = (types.Int8Binding, types.Int16Binding, types.Int32Binding,
                 types.Int64Binding, types.Uint8Binding, types.Uint16Binding,
                 types.Uint32Binding, types.Uint64Binding)
    FLOAT_TYPES = (types.FloatBinding, types.ComplexBinding)

    if isinstance(binding, (INT_TYPES, FLOAT_TYPES)):
        return binding.getMinMax()

    # XXX: raise an exception?
    return None, None


def get_numpy_binding(binding):
    """Retrieve a corresponding numpy `dtype` to a binding `binding`

    Note: In case of vector types, the appropriate `dtype` of an element
    is returned.
    """
    to_numpy_dtype = {
        types.Int8Binding: np.int8,
        types.Uint8Binding: np.uint8,
        types.Int16Binding: np.int16,
        types.Uint16Binding: np.uint16,
        types.Int32Binding: np.int32,
        types.Uint32Binding: np.uint32,
        types.Int64Binding: np.int64,
        types.Uint64Binding: np.uint64,
        types.FloatBinding: np.float64,
        types.ComplexBinding: np.complex128,
        types.VectorInt8Binding: np.int8,
        types.VectorUint8Binding: np.uint8,
        types.VectorInt16Binding: np.int16,
        types.VectorUint16Binding: np.uint16,
        types.VectorInt32Binding: np.int32,
        types.VectorUint32Binding: np.uint32,
        types.VectorInt64Binding: np.int64,
        types.VectorUint64Binding: np.uint64,
        types.VectorFloatBinding: np.float32,
        types.VectorDoubleBinding: np.float64,
        types.VectorComplexFloatBinding: np.complex64,
        types.VectorComplexDoubleBinding: np.complex128
    }
    return to_numpy_dtype.get(type(binding), None)


def get_native_min_max(binding):
    """Returns the appropriate numeric minimum and maximum for a binding

    Note: In of a vector binding the numeric limits of an element are returned

    This function neglects binding specific minimum and maximum
    """
    VECTOR_UNSIGNED = (
        types.VectorUint8Binding, types.VectorUint16Binding,
        types.VectorUint32Binding, types.VectorUint64Binding)
    VECTOR_SIGNED = (
        types.VectorInt8Binding, types.VectorInt16Binding,
        types.VectorInt32Binding, types.VectorInt64Binding)
    VECTOR_INTEGER_BINDINGS = VECTOR_SIGNED + VECTOR_UNSIGNED

    if isinstance(binding, (types.IntBinding, VECTOR_INTEGER_BINDINGS)):
        numpy = get_numpy_binding(binding)
        info = np.iinfo(numpy)
        return info.min, info.max

    elif isinstance(binding, (types.FloatBinding, types.VectorFloatBinding,
                              types.VectorComplexDoubleBinding)):
        numpy = get_numpy_binding(binding)
        info = np.finfo(numpy)
        return info.min, info.max

    return None, None


def get_min_max_size(binding):
    """Given a BaseBinding instance, return the minimum and maximum size
    which can be assigned to the vector trait
    """
    if isinstance(binding, types.VectorBinding):
        min_size = binding.attributes.get(const.KARABO_SCHEMA_MIN_SIZE)
        max_size = binding.attributes.get(const.KARABO_SCHEMA_MAX_SIZE)

        return min_size, max_size

    return None, None


def has_min_max_attributes(binding):
    """Check if there is a limit pair (inclusive, exclusive) in the attributes
    """
    attrs = binding.attributes
    min_inc = attrs.get(KARABO_SCHEMA_MIN_INC)
    min_exc = attrs.get(KARABO_SCHEMA_MIN_EXC)
    max_inc = attrs.get(KARABO_SCHEMA_MAX_INC)
    max_exc = attrs.get(KARABO_SCHEMA_MAX_EXC)

    return ((min_inc is not None or min_exc is not None) and
            (max_inc is not None or max_exc is not None))
