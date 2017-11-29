import contextlib

from karabo.middlelayer import (
    Configurable, Int8, Int16, UInt32, UInt64, String)
from ..api import (
    BaseBindingController, StringBinding, IntBinding, build_binding,
    register_binding_controller, get_compatible_controllers
)


class SomeObject(Configurable):
    for_display = String(options=['foo', 'bar', 'baz'])
    for_editing = String()
    unsupported = Int8()


class Integers(Configurable):
    int8 = Int8()
    int16 = Int16()
    uint32 = UInt32()
    uint64 = UInt64()


def _options_checker(binding):
    return len(binding.options) > 0


@contextlib.contextmanager
def _flushed_registry():
    """Avoid polluting the global registry with test controller classes"""
    from .. import registry as registrymod

    saved_registry = registrymod._controller_registry.copy()
    registrymod._controller_registry.clear()
    yield
    registrymod._controller_registry = saved_registry


def test_registry():
    with _flushed_registry():
        @register_binding_controller(binding_type=StringBinding,
                                     is_compatible=_options_checker)
        class DisplayWidget(BaseBindingController):
            pass

        @register_binding_controller(binding_type=StringBinding)
        class EditWidget(BaseBindingController):
            pass

        schema = SomeObject.getClassSchema()
        binding = build_binding(schema)

        bind = binding.value.for_display
        for_display_widgets = get_compatible_controllers(bind)
        assert DisplayWidget in for_display_widgets
        assert EditWidget in for_display_widgets

        bind = binding.value.for_editing
        for_editing_widgets = get_compatible_controllers(bind)
        assert EditWidget in for_editing_widgets
        assert DisplayWidget not in for_editing_widgets

        bind = binding.value.unsupported
        assert get_compatible_controllers(bind) == []


def test_expanded_registry():
    with _flushed_registry():
        @register_binding_controller(binding_type=IntBinding)
        class DisplayWidget(BaseBindingController):
            pass

        schema = Integers.getClassSchema()
        binding = build_binding(schema)

        for name in ('int8', 'int16', 'uint32', 'uint64'):
            bind = getattr(binding.value, name)
            assert DisplayWidget in get_compatible_controllers(bind)
