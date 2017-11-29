from nose.tools import assert_raises

from ..api import (
    BindingNamespace,
    BoolBinding, ByteArrayBinding, CharBinding, ChoiceOfNodesBinding,
    ComplexBinding, FloatBinding, HashBinding, Int8Binding, Int16Binding,
    Int32Binding, Int64Binding, ListOfNodesBinding, NodeBinding, NoneBinding,
    SchemaBinding, SlotBinding, StringBinding, Uint8Binding, Uint16Binding,
    Uint32Binding, Uint64Binding, VectorBoolBinding, VectorCharBinding,
    VectorComplexDoubleBinding, VectorComplexFloatBinding, VectorDoubleBinding,
    VectorFloatBinding, VectorHashBinding, VectorInt8Binding,
    VectorInt16Binding, VectorInt32Binding, VectorInt64Binding,
    VectorNoneBinding, VectorStringBinding, VectorUint8Binding,
    VectorUint16Binding, VectorUint32Binding, VectorUint64Binding
)

ALL_BINDINGS = (
    BoolBinding, ByteArrayBinding, CharBinding, ChoiceOfNodesBinding,
    ComplexBinding, FloatBinding, HashBinding, Int8Binding, Int16Binding,
    Int32Binding, Int64Binding, ListOfNodesBinding, NodeBinding, NoneBinding,
    SchemaBinding, SlotBinding, StringBinding, Uint8Binding, Uint16Binding,
    Uint32Binding, Uint64Binding, VectorBoolBinding, VectorCharBinding,
    VectorComplexDoubleBinding, VectorComplexFloatBinding, VectorDoubleBinding,
    VectorFloatBinding, VectorHashBinding, VectorInt8Binding,
    VectorInt16Binding, VectorInt32Binding, VectorInt64Binding,
    VectorNoneBinding, VectorStringBinding, VectorUint8Binding,
    VectorUint16Binding, VectorUint32Binding, VectorUint64Binding
)


def test_binding_traits():
    # Binding models must only have these traits!
    expected_traits = ('attributes', 'modified', 'timestamp', 'value')
    for binding_factory in ALL_BINDINGS:
        obj = binding_factory()
        for trait in obj.copyable_trait_names():
            assert trait in expected_traits


def test_binding_namespace():
    ns = BindingNamespace()

    assert list(ns) == []

    ns.foo = 0
    ns.bar = 'baz'
    assert list(ns) == ['foo', 'bar']

    del ns.bar
    assert list(ns) == ['foo']
    assert repr(ns) == "{\n\tfoo: 0\n}"

    ints_only = BindingNamespace(item_type=int)
    with assert_raises(ValueError):
        ints_only.not_an_int = 3.14
