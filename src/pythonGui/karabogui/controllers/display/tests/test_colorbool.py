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
from qtpy.QtWidgets import QWidget

from karabo.common.api import State
from karabo.common.scenemodel.api import ColorBoolModel
from karabo.native import Bool, Configurable, Hash
from karabogui.binding.api import apply_configuration
from karabogui.indicators import STATE_COLORS
from karabogui.testing import get_class_property_proxy

from ..colorbool import DisplayColorBool


class MockQSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class Object(Configurable):
    prop = Bool(defaultValue=True)


def test_colorbool(gui_app, mocker):
    # set up
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    model = ColorBoolModel()
    target = "karabogui.controllers.display.colorbool.SvgWidget"
    mocker.patch(target, new=MockQSvgWidget)
    proxy.value = False

    # test basics
    controller = DisplayColorBool(proxy=proxy, model=model)
    controller.create(None)
    assert controller.widget is not None

    # exercise code paths
    apply_configuration(Hash("prop", True),
                        proxy.root_proxy.binding)
    active = controller.icon.with_color(STATE_COLORS[State.ACTIVE])
    assert controller.widget.loaded_data == bytearray(active, encoding="UTF-8")

    apply_configuration(Hash("prop", False), proxy.root_proxy.binding)
    passive = controller.icon.with_color(STATE_COLORS[State.PASSIVE])
    assert controller.widget.loaded_data == bytearray(passive,
                                                      encoding="UTF-8")

    # teardown
    controller.destroy()
    assert controller.widget is None
