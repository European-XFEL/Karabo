
from karabo.middlelayer import Configurable, String, AccessMode
from ..api import (
    BaseBindingWidget, StringBinding, build_binding, register_binding_widget,
    get_compatible_widgets
)


class SomeObject(Configurable):
    for_display = String(accessMode=AccessMode.READONLY,
                         options=['foo', 'bar', 'baz'])
    unsupported = String(accessMode=AccessMode.READONLY)
    for_editing = String(accessMode=AccessMode.RECONFIGURABLE)


def options_checker(binding):
    return len(binding.options) > 0


@register_binding_widget(binding_type=StringBinding, read_only=True,
                         is_compatible=options_checker)
class DisplayWidget(BaseBindingWidget):
    pass


@register_binding_widget(binding_type=StringBinding, read_only=False)
class EditWidget(BaseBindingWidget):
    pass


def test_registry():
    schema = SomeObject.getClassSchema()
    binding = build_binding(schema)

    for_display_widgets = get_compatible_widgets(binding.value.for_display)
    assert DisplayWidget in for_display_widgets
    assert EditWidget not in for_display_widgets

    for_editing_widgets = get_compatible_widgets(binding.value.for_editing)
    assert EditWidget in for_editing_widgets
    assert DisplayWidget not in for_editing_widgets

    assert get_compatible_widgets(binding.value.unsupported) == []
