# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
