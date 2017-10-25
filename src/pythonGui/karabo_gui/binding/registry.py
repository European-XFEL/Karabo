from collections import defaultdict
from collections.abc import Iterable

from traits.api import Constant

from karabo.middlelayer import AccessMode
from .widget import BaseBindingWidget

# Module global registry
_widget_registry = defaultdict(set)


def get_compatible_widgets(binding_instance):
    """Return a list of `BaseBindingWidget` subclasses which are capable of
    visualizing the given `BaseBinding` object instance
    """
    def _get_class_trait(klass, name):
        trait = klass.class_traits()[name]
        return trait.default

    def check_compatibility(klass):
        read_only = binding_instance.access_mode is AccessMode.READONLY
        if read_only == _get_class_trait(klass, '_read_only'):
            return _get_class_trait(klass, '_is_compatible')(binding_instance)
        return False

    key = type(binding_instance)
    # XXX: Sort based on priority?
    klasses = sorted(_widget_registry[key], key=lambda x: x.__name__)
    # Give the classes a chance to reject
    klasses = [klass for klass in klasses if check_compatibility(klass)]
    return klasses


class register_binding_widget(object):
    """A class decorator for `BaseBindingWidget` subclasses which registers
    the widget for the various UI builders and adds some useful metadata.
    """
    def __init__(self, *, ui_name="", binding_type=(), read_only=False,
                 is_compatible=None):
        if not isinstance(binding_type, Iterable):
            binding_type = (binding_type,)

        self.ui_name = str(ui_name)
        self.binding_type = tuple(binding_type)
        self.read_only = bool(read_only)
        self.is_compatible = is_compatible or (lambda binding: True)

    def __call__(self, klass):
        assert issubclass(klass, BaseBindingWidget)

        binding_type_trait = Constant(self.binding_type)
        klass.add_class_trait('_binding_type', binding_type_trait)
        klass.add_class_trait('_ui_name', Constant(self.ui_name))
        klass.add_class_trait('_read_only', Constant(self.read_only))
        klass.add_class_trait('_is_compatible', Constant(self.is_compatible))

        # Register `klass` for building UIs
        for t in self.binding_type:
            _widget_registry[t].add(klass)
        return klass
