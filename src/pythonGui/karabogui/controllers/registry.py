from collections import defaultdict
from collections.abc import Iterable
from itertools import chain

from traits.api import Constant

from karabo.common.scenemodel.api import BaseWidgetObjectData
from .base import BaseBindingController

# Module global registry
_controller_registry = defaultdict(set)
_controller_models = defaultdict(set)


def get_class_const_trait(klass, name):
    """Return the value of a `Constant` trait which has been added to a class
    object by `register_binding_controller`.
    """
    trait = klass.class_traits()[name]
    return trait.default


def get_compatible_controllers(binding_instance, can_edit=False):
    """Returns a list of `BaseBindingController` subclasses which are capable
    of creating a view for the given `BaseBinding` object instance.
    """
    def check_compatibility(klass):
        if can_edit == get_class_const_trait(klass, '_can_edit'):
            checker = get_class_const_trait(klass, '_is_compatible')
            return checker(binding_instance)
        return False

    key = type(binding_instance)
    # Sort based on priority
    klasses = sorted(_controller_registry[key],
                     key=lambda x: get_class_const_trait(x, '_priority'))
    # Give the classes a chance to reject
    klasses = [klass for klass in klasses if check_compatibility(klass)]
    return klasses


def get_model_controller(scene_model):
    """Given a scene model instance, return the appropriate controller class
    to use in the scene view.
    """
    def _with_attribute(controllers, attr_name, attr_value):
        for klass in controllers:
            if attr_value == get_class_const_trait(klass, attr_name):
                return klass

    will_edit = scene_model.parent_component != 'DisplayComponent'
    controller_klass = getattr(scene_model, 'klass', '')
    klasses = _controller_models.get(type(scene_model), [])

    if controller_klass != '':
        # If a class name is specified by the model, use that
        return _with_attribute(klasses, '_klassname', controller_klass)
    elif len(klasses) > 1:
        # Use the editability to differentiate
        return _with_attribute(klasses, '_can_edit', will_edit)
    else:
        # We have to return the only controller that was found.
        # `klasses` might be a `set` and sets don't allow indexing
        return next(iter(klasses))


def get_scene_model_class(klass):
    """Return the scene model class for a given `BaseBindingController`
    subclass which has been registered.
    """
    trait = klass.class_traits()['model']
    instance_trait = trait.trait_type

    # Controller classes MUST define their scene model class!
    assert instance_trait.klass is not BaseWidgetObjectData
    return instance_trait.klass


class register_binding_controller(object):
    """A class decorator for `BaseBindingController` subclasses which registers
    the widget for the various UI builders and adds some useful metadata.
    """
    def __init__(self, *, ui_name='', klassname='', binding_type=(),
                 can_edit=False, is_compatible=None, priority=0):
        assert klassname != '', '`klassname` must be provided!'

        # Expand all binding types into a set of all possible subclasses
        if not isinstance(binding_type, Iterable):
            binding_type = set(_expand_binding(binding_type))
        else:
            accum = [_expand_binding(t) for t in binding_type]
            binding_type = set(chain(*accum))

        self.ui_name = str(ui_name)
        self.klassname = str(klassname)
        self.binding_type = tuple(binding_type)
        self.can_edit = bool(can_edit)
        self.is_compatible = is_compatible or (lambda binding: True)
        self.priority = priority

    def __call__(self, klass):
        assert issubclass(klass, BaseBindingController)

        binding_type_trait = Constant(self.binding_type)
        klass.add_class_trait('_binding_type', binding_type_trait)
        klass.add_class_trait('_ui_name', Constant(self.ui_name))
        klass.add_class_trait('_klassname', Constant(self.klassname))
        klass.add_class_trait('_can_edit', Constant(self.can_edit))
        klass.add_class_trait('_is_compatible', Constant(self.is_compatible))
        klass.add_class_trait('_priority', Constant(self.priority))

        # Register `klass` for building UIs
        model_klass = get_scene_model_class(klass)
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
