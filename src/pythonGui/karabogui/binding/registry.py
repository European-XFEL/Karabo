from collections import defaultdict
from collections.abc import Iterable
from itertools import chain

from traits.api import Constant

from .controller import BaseBindingController

# Module global registry
_controller_registry = defaultdict(set)


def _expand_binding(binding_klass):
    """Given a binding class, generate all the classes descended from that
    class.
    """
    yield binding_klass
    for klass in binding_klass.__subclasses__():
        yield from _expand_binding(klass)


def get_compatible_controllers(binding_instance, read_only=False):
    """Returns a list of `BaseBindingController` subclasses which are capable
    of creating a view for the given `BaseBinding` object instance.
    """
    def _get_class_trait(klass, name):
        trait = klass.class_traits()[name]
        return trait.default

    def check_compatibility(klass):
        if read_only == _get_class_trait(klass, '_read_only'):
            return _get_class_trait(klass, '_is_compatible')(binding_instance)
        return False

    key = type(binding_instance)
    # XXX: Sort based on priority?
    klasses = sorted(_controller_registry[key], key=lambda x: x.__name__)
    # Give the classes a chance to reject
    klasses = [klass for klass in klasses if check_compatibility(klass)]
    return klasses


class register_binding_controller(object):
    """A class decorator for `BaseBindingController` subclasses which registers
    the widget for the various UI builders and adds some useful metadata.
    """
    def __init__(self, *, ui_name="", binding_type=(), read_only=False,
                 is_compatible=None):
        # Expand all binding types into a set of all possible subclasses
        if not isinstance(binding_type, Iterable):
            binding_type = set(_expand_binding(binding_type))
        else:
            accum = [_expand_binding(t) for t in binding_type]
            binding_type = set(chain(*accum))

        self.ui_name = str(ui_name)
        self.binding_type = tuple(binding_type)
        self.read_only = bool(read_only)
        self.is_compatible = is_compatible or (lambda binding: True)

    def __call__(self, klass):
        assert issubclass(klass, BaseBindingController)

        binding_type_trait = Constant(self.binding_type)
        klass.add_class_trait('_binding_type', binding_type_trait)
        klass.add_class_trait('_ui_name', Constant(self.ui_name))
        klass.add_class_trait('_read_only', Constant(self.read_only))
        klass.add_class_trait('_is_compatible', Constant(self.is_compatible))

        # Register `klass` for building UIs
        for t in self.binding_type:
            _controller_registry[t].add(klass)
        return klass
