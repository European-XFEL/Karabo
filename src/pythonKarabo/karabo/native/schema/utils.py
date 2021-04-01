from karabo.common.const import (
    is_boolean_type, is_bytearray_type, is_integer_type, is_float_type,
    is_string_type, is_vector_type, is_vector_char_type, is_vector_float_type,
    is_vector_integer_type, KARABO_TYPE_VECTOR_BOOL, KARABO_TYPE_VECTOR_STRING)

from karabo.native import AccessMode, Hash, Schema

__all__ = ['get_default_value', 'get_value_type_default',
           'sanitize_table_schema']


def get_default_value(descriptor, force=False):
    """Return the default value for a descriptor

    :param descriptor: A descriptor like class
    """
    default = getattr(descriptor, "defaultValue")
    if default is None and force:
        default = get_value_type_default(descriptor.hashname())

    return default


def get_value_type_default(vtype: str):
    """Return the default value for a value type

    : param vtype: The string describing the value type

    Returns the appropriate default value for a karabo value type.
    """
    if is_string_type(vtype):
        return ""
    elif is_integer_type(vtype):
        return 0
    elif is_float_type(vtype):
        return 0.0
    elif is_vector_type(vtype):
        # All vectors including table!
        return []
    elif is_boolean_type(vtype):
        return False
    elif is_bytearray_type(vtype):
        return bytearray([])
    elif is_vector_char_type(vtype):
        return bytes()

    text = f"The value type {vtype} is not supported for a default value"
    raise NotImplementedError(text)


def sanitize_table_schema(schema: Schema, readonly: bool) -> Schema:
    """Sanitize a table row schema with default values and access mode"""
    success = True

    # Schema Hashes are empty!
    for key, value, attrs in Hash.flat_iterall(schema.hash, empty=True):
        # 1. Check defaultValue for writable tables
        if not readonly and attrs.get("defaultValue", None) is None:
            success = False
            vtype = attrs["valueType"]
            attrs.update({"defaultValue": get_value_type_default(vtype)})

        # 2. AccessMode. `None` happens in test cases. Schemas do not have
        # enums, but values
        access = attrs.get("accessMode", None)
        if access == AccessMode.INITONLY.value:
            success = False
            attrs.update({"accessMode": AccessMode.RECONFIGURABLE.value})

        # 3. Check valid types
        vtype = attrs["valueType"]
        valid = (is_boolean_type(vtype) | is_float_type(vtype) |
                 is_integer_type(vtype) | is_string_type(vtype) |
                 is_vector_integer_type(vtype) | is_vector_float_type(vtype) |
                 (vtype == KARABO_TYPE_VECTOR_BOOL) |
                 (vtype == KARABO_TYPE_VECTOR_STRING))
        if not valid:
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
            "Patching the schema to add defaults and changed INITONLY "
            "to RECONFIGURABLE. INITONLY properties in Table elements "
            "are deprecated.",
            stacklevel=2)

    return schema
