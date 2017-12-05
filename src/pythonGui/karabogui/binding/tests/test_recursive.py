from nose.tools import assert_raises
from traits.api import TraitError

from karabo.middlelayer import Hash
from ..api import (
    BindingNamespace, BindingRoot, ChoiceOfNodesBinding, FloatBinding,
    Int32Binding, ListOfNodesBinding, NodeBinding,
    apply_configuration, apply_default_configuration,
    build_binding, extract_configuration, extract_attribute_modifications,
    KARABO_SCHEMA_DEFAULT_VALUE
)
from ..recursive import duplicate_binding
from .schema import get_recursive_schema


def test_choice_of_nodes():
    foo_ns = BindingNamespace()
    foo_ns.ent = Int32Binding(value=42)
    Foo = BindingRoot(class_id='Foo', value=foo_ns)

    bar_ns = BindingNamespace()
    bar_ns.pie = FloatBinding(value=3.1415)
    Bar = BindingRoot(class_id='Bar', value=bar_ns)

    namespace = BindingNamespace()
    namespace.Foo = Foo
    namespace.Bar = Bar
    choice = ChoiceOfNodesBinding(choices=namespace)

    # Default value is the first node in the namespace, ie Foo
    assert 'ent' in choice.value
    assert isinstance(choice.value.ent, Int32Binding)

    # Now assign a value
    choice.value = 'Bar'
    assert 'pie' in choice.value
    assert isinstance(choice.value.pie, FloatBinding)


def test_list_of_nodes():
    foo_ns = BindingNamespace()
    foo_ns.ent = Int32Binding(value=42)
    node_ns = BindingNamespace()
    node_ns.problems = Int32Binding(value=99)
    foo_ns.node = NodeBinding(value=node_ns)
    Foo = BindingRoot(class_id='Foo', value=foo_ns)

    bar_ns = BindingNamespace()
    bar_ns.pie = FloatBinding(value=3.1415)
    Bar = BindingRoot(class_id='Bar', value=bar_ns)

    namespace = BindingNamespace()
    namespace.Foo = Foo
    namespace.Bar = Bar
    lonny = ListOfNodesBinding(choices=namespace)

    r_namespace = BindingNamespace()
    r_namespace.LoN = lonny
    choices = BindingNamespace()
    choices.root = BindingRoot(class_id='root', value=r_namespace)
    recursive = ListOfNodesBinding(choices=choices)

    # Default value is an empty list
    assert lonny.value == []

    # Append a string
    lonny.value.append('Bar')
    assert isinstance(lonny.value[-1], BindingRoot)
    assert lonny.value[-1].class_id == 'Bar'

    # Append a Hash
    bar = Hash('Bar', extract_configuration(Bar))
    lonny.value.append(bar)
    assert isinstance(lonny.value[-1], BindingRoot)
    assert lonny.value[-1].class_id == 'Bar'

    # Append a BindingRoot
    lonny.value.append(duplicate_binding(Foo))
    assert isinstance(lonny.value[-1], BindingRoot)
    assert lonny.value[-1].class_id == 'Foo'

    # Add garbage
    with assert_raises(TraitError):
        lonny.value.append('NotInChoices')

    # Test duplication of ListOfNodes
    recursive.value.append('root')
    first = recursive.value[-1]
    assert first.class_id == 'root'
    assert 'LoN' in first.value
    assert isinstance(first.value.LoN, ListOfNodesBinding)
    assert first.value.LoN.value == []
    first.value.LoN.value.append('Bar')
    assert first.value.LoN.value[-1].class_id == 'Bar'


def test_choice_of_nodes_configuration():
    schema = get_recursive_schema()
    binding = build_binding(schema)

    # choice is a _NodeTwo
    binding.value.con.value = '_NodeTwo'
    config = Hash("con", Hash("one", "hello"))
    apply_configuration(config, binding)
    assert binding.value.con.value.one.value == "hello"

    # Pass a configuration containing a _NodeOne value
    config = Hash("con", Hash("zero", "nope"))
    apply_configuration(config, binding)
    # Nothing happened here!
    assert 'zero' not in binding.value.con.value
    assert binding.value.con.value.one.value == "hello"

    # Choose _NodeOne, so it works this time.
    binding.value.con.value = '_NodeOne'
    apply_configuration(config, binding)
    assert binding.value.con.value.zero.value == "nope"


def test_list_of_nodes_configuration():
    schema = get_recursive_schema()
    binding = build_binding(schema)

    config = Hash("lon", [Hash("_NodeOne", Hash("zero", "hello")),
                          Hash("_NodeTwo", Hash("one", "world"))])
    apply_configuration(config, binding)
    assert binding.value.lon.value[0].value.zero.value == "hello"
    assert binding.value.lon.value[1].value.one.value == "world"


def test_apply_default_configuration():
    schema = get_recursive_schema()
    binding = build_binding(schema)

    apply_default_configuration(binding)
    # default choice is `_NodeTwo`, it should have attr 'one'
    assert hasattr(binding.value.con.value, 'one')
    assert binding.value.con.value.one.value == 'Second'

    # default list only contain `_NodeOne`, it has attr 'zero'
    assert len(binding.value.lon.value) == 1
    assert hasattr(binding.value.lon.value[0].value, 'zero')
    assert binding.value.lon.value[0].value.zero.value == 'First'


def test_extract_configuration():
    schema = get_recursive_schema()
    binding = build_binding(schema)

    apply_default_configuration(binding)
    ret = extract_configuration(binding)

    assert 'con' in ret
    assert 'lon' in ret
    assert ret['con'] == Hash('one', 'Second')
    assert ret['lon'] == [Hash('_NodeOne', Hash('zero', 'First'))]


def test_extract_attribute_modifications():
    schema = get_recursive_schema()
    binding = build_binding(schema)

    apply_default_configuration(binding)
    ret = extract_attribute_modifications(schema, binding)
    # no changes
    assert ret is None

    # Change defaultValue from _NodeTwo to _NodeOne
    binding.value.con.attributes[KARABO_SCHEMA_DEFAULT_VALUE] = '_NodeOne'
    ret = extract_attribute_modifications(schema, binding)
    assert ret[0] == Hash('path', 'con',
                          'attribute', KARABO_SCHEMA_DEFAULT_VALUE,
                          'value', '_NodeOne')

    binding = build_binding(schema)
    binding.value.lon.attributes[KARABO_SCHEMA_DEFAULT_VALUE] = ['_NodeOne',
                                                                 '_NodeTwo']
    ret = extract_attribute_modifications(schema, binding)
    assert ret[0] == Hash('path', 'lon',
                          'attribute', KARABO_SCHEMA_DEFAULT_VALUE,
                          'value', ['_NodeOne', '_NodeTwo'])
