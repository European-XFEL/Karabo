from karabo.native import Configurable, String
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..lineedit import DisplayLineEdit


class Object(Configurable):
    prop = String()


def test_set_string_value(gui_app):
    # set up
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = DisplayLineEdit(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None

    # body
    set_proxy_value(proxy, "prop", "hello")
    assert controller.widget.text() == "hello"

    # teardown
    controller.destroy()
    assert controller.widget is None
