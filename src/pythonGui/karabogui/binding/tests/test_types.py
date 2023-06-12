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
from pytest import raises as assert_raises

from karabo.native import AccessLevel, AccessMode, Assignment, Hash
from karabogui.binding.api import (
    BindingNamespace, BoolBinding, ByteArrayBinding, CharBinding,
    ChoiceOfNodesBinding, ComplexBinding, FloatBinding, HashBinding,
    Int8Binding, Int16Binding, Int32Binding, Int64Binding, ListOfNodesBinding,
    NDArrayBinding, NodeBinding, NoneBinding, SchemaBinding, SlotBinding,
    StringBinding, Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding,
    VectorBoolBinding, VectorCharBinding, VectorComplexDoubleBinding,
    VectorComplexFloatBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorHashBinding, VectorInt8Binding, VectorInt16Binding,
    VectorInt32Binding, VectorInt64Binding, VectorNoneBinding,
    VectorStringBinding, VectorUint8Binding, VectorUint16Binding,
    VectorUint32Binding, VectorUint64Binding)

ALL_BINDINGS = (
    BoolBinding, ByteArrayBinding, CharBinding, ChoiceOfNodesBinding,
    ComplexBinding, FloatBinding, HashBinding, Int8Binding, Int16Binding,
    Int32Binding, Int64Binding, ListOfNodesBinding, NDArrayBinding,
    NodeBinding, NoneBinding,
    SchemaBinding, SlotBinding, StringBinding, Uint8Binding, Uint16Binding,
    Uint32Binding, Uint64Binding, VectorBoolBinding, VectorCharBinding,
    VectorComplexDoubleBinding, VectorComplexFloatBinding, VectorDoubleBinding,
    VectorFloatBinding, VectorHashBinding, VectorInt8Binding,
    VectorInt16Binding, VectorInt32Binding, VectorInt64Binding,
    VectorNoneBinding, VectorStringBinding, VectorUint8Binding,
    VectorUint16Binding, VectorUint32Binding, VectorUint64Binding
)


def test_binding_traits():
    # Defined trait names in Binding models,
    # children_names should not be used for binding types whose value are of
    # type BindingNamespace
    expected_traits = ('_attributes', 'access_mode', 'assignment',
                       'attributes', 'children_names', 'options',
                       'required_access_level', 'timestamp', 'unit_label',
                       'value', 'displayed_name', 'display_type', 'row_schema')
    for binding_factory in ALL_BINDINGS:
        obj = binding_factory()
        for trait in obj.copyable_trait_names():
            assert trait in expected_traits

        assert obj.required_access_level is AccessLevel.OBSERVER
        assert obj.displayed_name == ""
        assert obj.display_type == ""
        assert obj.access_mode is AccessMode.UNDEFINED
        assert obj.assignment is Assignment.OPTIONAL
        assert not obj.options
        assert obj.unit_label == ""


def test_binding_namespace():
    ns = BindingNamespace()

    assert list(ns) == []

    ns.foo = 0
    ns.bar = 'baz'
    assert list(ns) == ['foo', 'bar']

    del ns.bar
    assert list(ns) == ['foo']
    assert repr(ns) == "{\n\tfoo: 0\n}"

    ns.clear_namespace()
    assert list(ns) == []

    with assert_raises(AttributeError):
        del ns.foo

    ints_only = BindingNamespace(item_type=int)
    with assert_raises(ValueError):
        ints_only.not_an_int = 3.14


def test_table_binding():
    vector_hash = VectorHashBinding()
    assert vector_hash.row_schema == Hash()
    vector_hash.update_attributes({})
    assert vector_hash.row_schema == Hash()
