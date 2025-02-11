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
from traits.api import Undefined

from karabo.common import const
from karabo.native import Hash, NodeType

from . import binding_types as types
from .util import attr_fast_deepcopy


def build_binding(schema, existing=None):
    """Given a schema object, build an object binding which matches the
    structure defined in the schema. All leaf nodes are built with an initial
    value of Undefined.
    """
    # Use `existing` or create a new instance.
    binding = existing or types.BindingRoot()
    binding.class_id = schema.name
    root_namespace = binding.value
    # Clear the namespace
    root_namespace.clear_namespace()
    # Fill it back in recursively
    for key, value, attrs in schema.hash.iterall():
        node = _build_node(value, attrs)
        setattr(root_namespace, key, node)

    # Let any listeners know
    binding.schema_update = True

    return binding


def _build_node(value, attrs):
    """Recursively build a single node (and its children) in an object binding
    """
    # Always deal with a copy of the attributes
    # (to allow bindings to modify)
    attrs = attr_fast_deepcopy(attrs)

    def _build_subnamespace(hashval, item_type):
        assert isinstance(hashval, Hash)
        namespace = types.BindingNamespace(item_type=types.BaseBinding)
        for subname, subvalue, subattrs in hashval.iterall():
            subnode = _build_node(subvalue, subattrs)
            setattr(namespace, subname, subnode)
        return namespace

    node_type = attrs[const.KARABO_SCHEMA_NODE_TYPE]
    if node_type == NodeType.Leaf:
        value_type = attrs[const.KARABO_SCHEMA_VALUE_TYPE]
        binding_factory = _BINDING_MAP[value_type]
        return binding_factory(attributes=attrs, value=Undefined)

    elif node_type == NodeType.Node:
        display_type = attrs.get(
            const.KARABO_SCHEMA_DISPLAY_TYPE, '').split('|')[0]
        namespace = _build_subnamespace(value, types.BaseBinding)
        if display_type in _NODE_BINDING_MAP:
            # Certain display types get a different binding class
            binding_factory = _NODE_BINDING_MAP[display_type]
            return binding_factory(value=namespace, attributes=attrs)
        return types.NodeBinding(value=namespace, attributes=attrs)

    raise RuntimeError(f"Not supported value type {node_type}")


# ----------------------------------------------------------------------------
# A dict to help with valueType -> binding type mapping
_BINDING_MAP = {
    'BOOL': types.BoolBinding,
    'CHAR': types.CharBinding,
    'INT8': types.Int8Binding,
    'UINT8': types.Uint8Binding,
    'INT16': types.Int16Binding,
    'UINT16': types.Uint16Binding,
    'INT32': types.Int32Binding,
    'UINT32': types.Uint32Binding,
    'INT64': types.Int64Binding,
    'UINT64': types.Uint64Binding,
    'FLOAT': types.FloatBinding,
    'DOUBLE': types.FloatBinding,
    'COMPLEX_FLOAT': types.ComplexBinding,
    'COMPLEX_DOUBLE': types.ComplexBinding,
    'STRING': types.StringBinding,
    'HASH': types.HashBinding,
    'SCHEMA': types.SchemaBinding,
    'NONE': types.NoneBinding,
    'BYTE_ARRAY': types.ByteArrayBinding,
    'VECTOR_BOOL': types.VectorBoolBinding,
    'VECTOR_CHAR': types.VectorCharBinding,
    'VECTOR_INT8': types.VectorInt8Binding,
    'VECTOR_UINT8': types.VectorUint8Binding,
    'VECTOR_INT16': types.VectorInt16Binding,
    'VECTOR_UINT16': types.VectorUint16Binding,
    'VECTOR_INT32': types.VectorInt32Binding,
    'VECTOR_UINT32': types.VectorUint32Binding,
    'VECTOR_INT64': types.VectorInt64Binding,
    'VECTOR_UINT64': types.VectorUint64Binding,
    'VECTOR_DOUBLE': types.VectorDoubleBinding,
    'VECTOR_FLOAT': types.VectorFloatBinding,
    'VECTOR_COMPLEX_DOUBLE': types.VectorComplexDoubleBinding,
    'VECTOR_COMPLEX_FLOAT': types.VectorComplexFloatBinding,
    'VECTOR_STRING': types.VectorStringBinding,
    'VECTOR_HASH': types.VectorHashBinding,
    'VECTOR_NONE': types.VectorNoneBinding,
}
_NODE_BINDING_MAP = {
    'ImageData': types.ImageBinding,
    'WidgetNode': types.WidgetNodeBinding,
    'Image': types.ImageBinding,
    'NDArray': types.NDArrayBinding,
    'Slot': types.SlotBinding,
    'OutputChannel': types.PipelineOutputBinding,
    'Table': types.TableBinding,
}
