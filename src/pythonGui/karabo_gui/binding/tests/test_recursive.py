from nose.tools import assert_raises
from traits.api import TraitError

from karabo.middlelayer import Hash
from ..api import (
    BindingNamespace, BindingRoot,
    ChoiceOfNodesBinding, FloatBinding, Int32Binding, ListOfNodesBinding,
    NodeBinding, extract_configuration
)
from ..recursive import duplicate_binding


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
