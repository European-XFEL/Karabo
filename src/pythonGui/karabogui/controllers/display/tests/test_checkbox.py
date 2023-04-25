# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.native import Bool, Configurable
from karabogui.testing import (
    check_renderer_against_svg, get_class_property_proxy, set_proxy_value)

from ..checkbox import CHECKED, UNCHECKED, DisplayCheckBox


class Object(Configurable):
    prop = Bool(defaultValue=True)


def test_checkbox_set_value(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, 'prop')
    controller = DisplayCheckBox(proxy=proxy)
    controller.create(None)

    set_proxy_value(proxy, 'prop', True)
    check_renderer_against_svg(controller.widget.renderer(),
                               CHECKED)

    set_proxy_value(proxy, 'prop', False)
    check_renderer_against_svg(controller.widget.renderer(),
                               UNCHECKED)

    controller.destroy()
    assert controller.widget is None
