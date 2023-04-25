# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from traits.api import Dict, Instance, List, Property, Trait, TraitHandler

from karabo.native import Hash

from .binding_types import (
    BaseBinding, BindingNamespace, BindingRoot, NodeBinding)
from .util import attr_fast_deepcopy


class ChoiceOfNodesBinding(BaseBinding):
    """A node whose type can be changed by assigning a new classname or Hash to
    its `value`. Valid classnames are stored in the `choices` namespace. When
    reading or traversing this node, it appears to be a normal node whose value
    is a `BindingNamespace` containing a single `BindingRoot` instance.
    """
    value = Property
    choice = Property(transient=True, depends_on='_value')
    # Namespace of possible values
    choices = Instance(BindingNamespace, kw={'item_type': BindingRoot},
                       transient=True)
    # Private detail
    _value = Instance(BindingNamespace, transient=True)
    # Visible children names {accesslevel: [names]}
    children_names = Dict()

    def _choice_changed(self):
        # When the choice is changed, clear the children name cache
        self.children_names = {}

    def __value_default(self):
        ns = BindingNamespace(item_type=BindingRoot)
        choice = next(iter(self.choices))
        setattr(ns, choice, getattr(self.choices, choice))
        return ns

    def _get_choice(self):
        return next(iter(self._value))

    def _set_choice(self, choice):
        assert isinstance(choice, str) and choice in self.choices
        self._value.clear_namespace()
        setattr(self._value, choice, getattr(self.choices, choice))
        self.config_update = True

    def _get_value(self):
        return self._value

    def _set_value(self, value):
        if isinstance(value, Hash):
            choice = next(iter(value))
        else:  # value is a str
            choice = value

        if choice not in self._value:
            self.choice = choice

        if isinstance(value, Hash):
            from .config import apply_configuration
            apply_configuration(value[choice], getattr(self._value, choice))


class _ListOfNodesHandler(TraitHandler):
    """Handle ListOfNodes value assignments
    """
    def validate(self, instance, name, value):
        from .config import apply_configuration, apply_default_configuration

        def _get_dupe_binding(klassname):
            binding = getattr(instance.choices, klassname, None)
            if binding is None:
                self.error(instance, name, value)
            return duplicate_binding(binding)

        if isinstance(value, str):
            duplicate = _get_dupe_binding(value)
            apply_default_configuration(duplicate)
            return duplicate
        elif isinstance(value, Hash):
            klassname = next(iter(value))
            duplicate = _get_dupe_binding(klassname)
            apply_configuration(value[klassname], duplicate)
            return duplicate
        elif isinstance(value, BindingRoot):
            if value.class_id in instance.choices:
                return value
        self.error(instance, name, value)


class ListOfNodesBinding(BaseBinding):
    """A node whose values is a list of objects (nodes). The types of the
    objects must be present in the `choices` namespace.

    Either string values or object values can be written to the binding value,
    but only object values will be stored (as BindingRoot instances). When a
    string is written, it will be interpreted as a classname in the `choices`
    namespace and an instance with a default configuration will be produced.
    When an object is written, either as a Hash or as a BindingRoot, its value
    will be validated against its corresponding class in `choices`.
    """
    value = List(Trait(_ListOfNodesHandler()))
    # Namespace of possible list values
    choices = Instance(BindingNamespace, kw={'item_type': BindingRoot},
                       transient=True)
    # Visible children names {accesslevel: [names]}
    children_names = Dict()

    def _value_changed(self):
        # When the value is changed, clear the children name cache
        self.children_names = {}


# =============================================================================
# Helper functions
#

def duplicate_binding(binding):
    """Given a `BindingRoot` object, create a new object which is a copy of
    the binding, but does not contain the source binding's value.
    """
    assert isinstance(binding, BindingRoot)
    duplicate = BindingRoot(class_id=binding.class_id)
    for name in binding.value:
        node = _duplicate_node(getattr(binding.value, name))
        setattr(duplicate.value, name, node)

    return duplicate


def _duplicate_node(node):
    """Duplicate a `BaseBinding` object
    """
    traits = {'attributes': attr_fast_deepcopy(node.attributes)}
    if isinstance(node, (ChoiceOfNodesBinding, ListOfNodesBinding)):
        choices = node.choices
        traits['choices'] = ns = BindingNamespace(item_type=BindingRoot)
        for name in choices:
            setattr(ns, name, duplicate_binding(getattr(choices, name)))

    klass = node.__class__
    dupe = klass(**traits)
    if isinstance(node, NodeBinding):
        dupe.value = BindingNamespace(item_type=BaseBinding)
        for name in node.value:
            subnode = _duplicate_node(getattr(node.value, name))
            setattr(dupe.value, name, subnode)

    return dupe
