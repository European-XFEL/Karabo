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
