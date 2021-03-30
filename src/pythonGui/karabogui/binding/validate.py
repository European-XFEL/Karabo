from ast import literal_eval

import numpy as np
from traits.api import TraitError

from karabo.common import const
from karabo.native import HashList, Hash
from karabogui.binding.compare import realign_hash, has_array_changes
from karabogui.binding.recursive import (
    ListOfNodesBinding, ChoiceOfNodesBinding)
from karabogui.binding.types import (
    BoolBinding, ByteArrayBinding, CharBinding, ComplexBinding, FloatBinding,
    IntBinding, NodeBinding, StringBinding, VectorBinding, VectorDoubleBinding,
    VectorFloatBinding, VectorHashBinding, VectorNumberBinding,
)

VECTOR_FLOAT_BINDINGS = (VectorFloatBinding, VectorDoubleBinding)
RECURSIVE_BINDINGS = (NodeBinding, ListOfNodesBinding,
                      ChoiceOfNodesBinding)


def sanitize_table_value(binding, value):
    """Sanitize a hash list `value` against existing vector hash `binding`

    :param binding: The existing `VectorHashBinding`
    :param value: the table value (`HashList`)

    :returns: success (bool), sanitized value (`HashList`)
    """
    if value is None:
        # Theoretically this should not happen. No `None` value is sent via
        # network.
        return [], False

    msg = "Expected a value of type `HashList`, got %s instead" % type(value)
    assert isinstance(value, (list, HashList)), msg

    # Provide a success information if we received a valid table!
    success = True

    def _sanitize_row(row_bindings, row_hash):
        """Validate a single row of the table"""
        nonlocal success

        ret = Hash()
        if list(row_bindings.keys()) != list(row_hash.keys()):
            row_hash = realign_hash(row_hash, reference=row_bindings.keys())

        for path, value in row_hash.items():
            binding = row_bindings.get(path, None)
            if binding is None:
                # Tables might lose a property in a row schema. This is fine
                # as we can simply ignore this case. We don't report this as
                # invalid but continue gracefully ...
                continue

            if value is None:
                # The property is a `None` value or is not existent, e.g.
                # a column can have been added.
                # Use the default value from the binding, if necessary force!
                value = get_default_value(binding, force=True)
                success = False
            else:
                try:
                    value = binding.validate_trait("value", value)
                    # Use the fast path validate
                except TraitError:
                    value = None
                # Note: This is of course critical, if an apple becomes an
                # orange, we should have at least an orange default value.
                if value is None:
                    value = get_default_value(binding, force=True)
                    success = False
            ret[path] = value

        return ret

    ret = HashList()
    for row_hash in value:
        ret.append(_sanitize_row(binding.bindings, row_hash))

    return success, ret


def validate_value(binding, value):
    if type(value) is str:
        # There might be cases that values are saved as strings. This typically
        # occurs for table elements.
        if not isinstance(binding, (StringBinding, CharBinding)):
            value = convert_string(value)
    try:
        if isinstance(binding, VectorNumberBinding):
            # Check if binding is a vector, then test array against its
            # expected `dtype`.
            casted_value = binding.check(value)
            if isinstance(binding, VECTOR_FLOAT_BINDINGS):
                value = np.array(value, dtype=casted_value.dtype)
            if not has_array_changes(value, casted_value):
                value = casted_value
            else:
                value = None
        elif isinstance(binding, VectorHashBinding):
            # VectorHashBinding is not a valid value
            value = None
        elif isinstance(binding, RECURSIVE_BINDINGS):
            # Nothing to do here! We automatically return the value
            pass
        else:
            # The value is a simple data type. We validate it with the binding
            # traits.
            value = binding.check(value)
    except (TraitError, TypeError, ValueError):
        # TraitError happens when traits cannot cast, e.g. str to bool
        # Validation has failed, we inform that there's no validated value
        value = None

    return value


def validate_table_value(binding, value):
    """Validate a hash list `value` against existing vectorhash binding

    :param binding: The existing `VectorHashBinding`
    :param value: the value to be validated (`HashList`)

    :return valid, invalid: (HashList) The values could contain [None, Hash()]
    """

    def _validate_row(row_bindings, row_hash):
        """Validate a single row of the table

        :param row_bindings: The `rowSchema` attribute binding
        :param row_hash: The hash to be validated, either `Hash` or `None`

        :returns:

            - valid: Either a `Hash` or `None`. `None` if validation failed.
            - invalid: Either a `Hash` or `None`
        """
        if row_hash is None:
            # If `row_hash` Hash is `None`, we report as invalid value
            return None, Hash()

        valid = Hash()
        # Check if order of the keys are respected...
        if list(row_bindings.keys()) != list(row_hash.keys()):
            row_hash = realign_hash(row_hash, reference=row_bindings.keys())

        for path, value in row_hash.items():
            binding = row_bindings.get(path, None)
            if binding is None:
                # Tables might lose a property in a row schema. This is fine
                # as we can simply ignore this case. We don't report this as
                # invalid but continue gracefully ...
                continue

            if value is None:
                # The property value is `None` or the value doesn't exist
                # (from `realign_hash`).
                # Try to use the default value from the binding!
                validated_value = get_default_value(binding)
            else:
                validated_value = validate_value(binding, value)

            if validated_value is None:
                # We report the invalid property
                return None, row_hash

            valid[path] = validated_value

        return valid, None

    # Set default values
    valid, invalid = HashList(), HashList()
    for row_hash in value:
        valid_row, invalid_row = _validate_row(binding.bindings, row_hash)
        if valid_row is not None:
            valid.append(valid_row)
        if invalid_row is not None:
            invalid.append(invalid_row)

    return valid, invalid


def get_default_value(binding, force=False):
    """Get the default value from a binding"""

    def _get_binding_default(binding):
        # Provide a default value for all leafType bindings of `binding`
        if isinstance(binding, (CharBinding, StringBinding)):
            return ""
        if isinstance(binding, IntBinding):
            # XXX: No min and max taken into account for now
            return 0
        elif isinstance(binding, FloatBinding):
            return 0.0
        elif isinstance(binding, VectorBinding):
            # All vectors including table!
            return []
        elif isinstance(binding, ComplexBinding):
            return 0.0
        elif isinstance(binding, BoolBinding):
            return False
        elif isinstance(binding, ByteArrayBinding):
            return bytearray([])

        return None

    attrs = binding.attributes
    value = attrs.get(const.KARABO_SCHEMA_DEFAULT_VALUE, None)
    if value is None and force:
        value = _get_binding_default(binding)
        assert value is not None, f"No default value for {type(binding)} ..."
    return value


def convert_string(value: str):
    """Try to convert the string value `value` to a literal"""
    try:
        return literal_eval(value)
    except (SyntaxError, ValueError):
        # Conversion of string to a literal failed, we return the value as is.
        return value
