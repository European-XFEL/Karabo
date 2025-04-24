# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import numpy as np

import karabo.common.const as const
from karabo.common.const import (
    is_boolean_type, is_bytearray_type, is_float_type, is_integer_type,
    is_string_type, is_vector_bool_type, is_vector_char_type,
    is_vector_float_type, is_vector_hash_type, is_vector_integer_type,
    is_vector_string_type, is_vector_type)
from karabo.native.data import AccessMode, Hash, Schema


def get_default_value(descriptor, force=False):
    """Return the default value for a descriptor

    :param descriptor: A descriptor like class
    """
    default = getattr(descriptor, "defaultValue")
    if default is None and force:
        default = _get_value_type_default(descriptor.hashname(),
                                          descriptor.attributes)

    return default


def get_value_type_numpy(vtype: str):
    """Retrieve a corresponding numpy `dtype` to a valueType `vtype`

    Note: In case of vector types, the appropriate `dtype` of an element
    is returned.
    """
    to_numpy_dtype = {
        const.KARABO_TYPE_INT8: np.int8,
        const.KARABO_TYPE_UINT8: np.uint8,
        const.KARABO_TYPE_INT16: np.int16,
        const.KARABO_TYPE_UINT16: np.uint16,
        const.KARABO_TYPE_INT32: np.int32,
        const.KARABO_TYPE_UINT32: np.uint32,
        const.KARABO_TYPE_INT64: np.int64,
        const.KARABO_TYPE_UINT64: np.uint64,
        const.KARABO_TYPE_FLOAT: np.float32,
        const.KARABO_TYPE_DOUBLE: np.float64,
        const.KARABO_TYPE_COMPLEX_FLOAT: np.complex64,
        const.KARABO_TYPE_COMPLEX_DOUBLE: np.complex128,
        const.KARABO_TYPE_VECTOR_INT8: np.int8,
        const.KARABO_TYPE_VECTOR_UINT8: np.uint8,
        const.KARABO_TYPE_VECTOR_INT16: np.int16,
        const.KARABO_TYPE_VECTOR_UINT16: np.uint16,
        const.KARABO_TYPE_VECTOR_INT32: np.int32,
        const.KARABO_TYPE_VECTOR_UINT32: np.uint32,
        const.KARABO_TYPE_VECTOR_INT64: np.int64,
        const.KARABO_TYPE_VECTOR_UINT64: np.uint64,
        const.KARABO_TYPE_VECTOR_FLOAT: np.float32,
        const.KARABO_TYPE_VECTOR_DOUBLE: np.float64,
        const.KARABO_TYPE_VECTOR_COMPLEX_FLOAT: np.complex64,
        const.KARABO_TYPE_VECTOR_COMPLEX_DOUBLE: np.complex128
    }
    return to_numpy_dtype.get(vtype, None)


def _get_value_type_default(vtype: str, attributes={}):
    """Private function to return the default value for a `vtype`
     with the attributes `attributes`

    :param vtype: The string describing the value type

    Returns the appropriate default value for a karabo value type.
    """

    def _get_simple_default(vtype, attrs):
        """Return a default value for a simple value type"""
        if is_integer_type(vtype):
            dtype = get_value_type_numpy(vtype)
            low = attrs.get(const.KARABO_SCHEMA_MIN_EXC)
            if low is not None:
                low += 1
            else:
                low = attrs.get(const.KARABO_SCHEMA_MIN_INC, 0)

            value = dtype(0)
            if value >= low:
                return value

            return dtype(low)

        elif is_float_type(vtype):
            dtype = get_value_type_numpy(vtype)
            info = np.finfo(dtype)
            low = attrs.get(const.KARABO_SCHEMA_MIN_EXC)
            if low is not None:
                low = low * (1 + np.sign(low) * info.eps) + info.tiny
            else:
                low = attrs.get(const.KARABO_SCHEMA_MIN_INC, 0)

            value = dtype(0)
            if value >= low:
                return value

            return dtype(low)

    def _get_vector_default(vtype, attrs):
        """ Provide a default value for vector binding `binding` considering
        minSize attribute"""
        min_size = attrs.get(const.KARABO_SCHEMA_MIN_SIZE)
        if min_size is None:
            return []
        if is_vector_hash_type(vtype):
            raise TypeError("Cannot construct a defaultValue for a vector hash"
                            "with minimumSize.")

        elif is_vector_integer_type(vtype) or is_vector_float_type(vtype):
            dtype = get_value_type_numpy(vtype)
            value = np.zeros(shape=(min_size,), dtype=dtype)
        elif is_vector_string_type(vtype):
            value = [""] * min_size
        else:
            value = []
        return value

    # Always check the options attribute first if present
    options = attributes.get("options", None)
    if options is not None:
        return options[0]

    if is_string_type(vtype):
        return ""
    elif is_integer_type(vtype) or is_float_type(vtype):
        return _get_simple_default(vtype, attributes)
    elif is_vector_type(vtype):
        return _get_vector_default(vtype, attributes)
    elif is_boolean_type(vtype):
        return False
    elif is_bytearray_type(vtype):
        return bytearray([])
    elif is_vector_char_type(vtype):
        return b''

    text = f"The value type {vtype} is not supported for a default value"
    raise NotImplementedError(text)


def sanitize_table_schema(schema: Schema, readonly: bool) -> Schema:
    """Sanitize a table row schema with default values and access mode

    :param schema: The table schema for sanitization
    :param readonly: Boolean to denote if the table is a read only table

    This function inserts missing defaultValues into the table schema if the
    table schema belong to a reconfigurable table element.

    AccessModes ``INITONLY`` are either changed to ``READONLY`` or
    ``RECONFIGURABLE`` depending on the `readonly` input parameter.
    """
    success = True

    def _valid_type(vtype):
        """Return if a valueType `vtype` is valid for a vector hash"""
        return (is_boolean_type(vtype) | is_float_type(vtype) |
                is_integer_type(vtype) | is_string_type(vtype) |
                is_vector_integer_type(vtype) | is_vector_float_type(vtype) |
                is_vector_bool_type(vtype) | is_vector_string_type(vtype))

    # Schema Hashes are empty!
    for key, value, attrs in Hash.flat_iterall(schema.hash, empty=True):
        # 1. Check defaultValue for writable tables
        if not readonly and attrs.get("defaultValue", None) is None:
            success = False
            vtype = attrs["valueType"]
            value = _get_value_type_default(vtype, attributes=attrs)
            attrs.update({"defaultValue": value})

        # 2. AccessMode. `None` happens in test cases. Schemas do not have
        # enums, but values
        access = attrs.get("accessMode", None)
        if access == AccessMode.INITONLY.value:
            success = False
            sanitized_access = (AccessMode.READONLY.value if readonly
                                else AccessMode.RECONFIGURABLE.value)
            attrs.update({"accessMode": sanitized_access})

        # 3. Check valid types
        vtype = attrs["valueType"]
        if not _valid_type(vtype):
            raise TypeError(f"The element with key {key} has a non-supported "
                            f"valueType {vtype}")

        # 4. Check forbidden attributes, e.g. RegexString and VectorRegex!
        if attrs.get("regex") is not None:
            raise TypeError(f"Regex elements {key} are not supported")

    if not success:
        import warnings
        warnings.warn(
            f"The rowSchema {schema.name} does not fully specify "
            "defaultValues or has accessMode INITONLY. "
            "Patching the schema to add defaults and changed INITONLY. "
            "INITONLY properties in Table elements are deprecated.",
            stacklevel=2)

    return schema
