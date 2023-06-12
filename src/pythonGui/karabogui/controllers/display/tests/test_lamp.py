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
from qtpy.QtWidgets import QLabel

from karabo.native import Configurable, String
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..lamp import LampWidget


class MockLabel(QLabel):
    def setPixmap(self, pixmap):
        self.pixmap = pixmap

    def setMaximumWidth(self, value):
        self.width = value

    def setMaximumHeight(self, value):
        self.height = value


class Object(Configurable):
    state = String()


def test_set_values(gui_app, mocker):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "state")
    target = "karabogui.controllers.display.lamp.QLabel"
    mocker.patch(target, new=MockLabel)

    states = ("CHANGING", "ACTIVE", "PASSIVE", "DISABLED", "STATIC",
              "RUNNING", "NORMAL", "ERROR", "INIT", "UNKNOWN")

    controller = LampWidget(proxy=proxy)
    controller.create(None)

    for state in states:
        set_proxy_value(proxy, "state", state)
        assert controller.widget.pixmap is not None
        controller.widget.pixmap = None
