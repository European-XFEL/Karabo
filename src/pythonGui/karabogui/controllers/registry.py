from collections import defaultdict
from collections.abc import Iterable
from itertools import chain

from traits.api import Constant

from karabo.common.scenemodel.api import BaseWidgetObjectData
from .base import BaseBindingController

# Module global registry
_controller_registry = defaultdict(set)
_controller_models = defaultdict(set)


def get_compatible_controllers(binding_instance, can_edit=False):
    """Returns a list of `BaseBindingController` subclasses which are capable
    of creating a view for the given `BaseBinding` object instance.
    """
    def check_compatibility(klass):
        if can_edit == _get_class_trait(klass, '_can_edit'):
            return _get_class_trait(klass, '_is_compatible')(binding_instance)
        return False

    key = type(binding_instance)
    # XXX: Sort based on priority?
    klasses = sorted(_controller_registry[key], key=lambda x: x.__name__)
    # Give the classes a chance to reject
    klasses = [klass for klass in klasses if check_compatibility(klass)]
    return klasses


def get_model_controller(scene_model):
    """Given a scene model instance, return the appropriate controller class
    to use in the scene view.
    """
    will_edit = scene_model.parent_component != 'DisplayComponent'
    klasses = _controller_models.get(type(scene_model))
    for klass in klasses:
        if will_edit == _get_class_trait(klass, '_can_edit'):
            return klass


class register_binding_controller(object):
    """A class decorator for `BaseBindingController` subclasses which registers
    the widget for the various UI builders and adds some useful metadata.
    """
    def __init__(self, *, ui_name="", binding_type=(), can_edit=False,
                 is_compatible=None):
        # Expand all binding types into a set of all possible subclasses
        if not isinstance(binding_type, Iterable):
            binding_type = set(_expand_binding(binding_type))
        else:
            accum = [_expand_binding(t) for t in binding_type]
            binding_type = set(chain(*accum))

        self.ui_name = str(ui_name)
        self.binding_type = tuple(binding_type)
        self.can_edit = bool(can_edit)
        self.is_compatible = is_compatible or (lambda binding: True)

    def __call__(self, klass):
        assert issubclass(klass, BaseBindingController)

        binding_type_trait = Constant(self.binding_type)
        klass.add_class_trait('_binding_type', binding_type_trait)
        klass.add_class_trait('_ui_name', Constant(self.ui_name))
        klass.add_class_trait('_can_edit', Constant(self.can_edit))
        klass.add_class_trait('_is_compatible', Constant(self.is_compatible))

        # Register `klass` for building UIs
        model_klass = _get_scene_model_class(klass)
        _controller_models[model_klass].add(klass)
        for t in self.binding_type:
            _controller_registry[t].add(klass)
        return klass


# --------------------------------------------------------------------------
# Private things

def _expand_binding(binding_klass):
    """Given a binding class, generate all the classes descended from that
    class.
    """
    yield binding_klass
    for klass in binding_klass.__subclasses__():
        yield from _expand_binding(klass)


def _get_class_trait(klass, name):
    trait = klass.class_traits()[name]
    return trait.default


def _get_scene_model_class(klass):
    trait = klass.class_traits()['model']
    instance_trait = trait.trait_type

    # Controller classes MUST define their scene model class!
    assert instance_trait.klass is not BaseWidgetObjectData
    return instance_trait.klass
