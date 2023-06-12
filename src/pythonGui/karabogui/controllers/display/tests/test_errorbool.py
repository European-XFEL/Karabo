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

from karabo.common.scenemodel.api import ErrorBoolModel
from karabo.native import Bool, Configurable, Hash
from karabogui.binding.api import apply_configuration
from karabogui.testing import get_class_property_proxy

from ..errorbool import ERROR_BOOL, OK_BOOL, DisplayErrorBool


class MockQSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class Object(Configurable):
    prop = Bool(defaultValue=True)


schema = Object.getClassSchema()
new_schema = Object.getClassSchema()


def test_errorbool_controller(gui_app, mocker):
    # setup
    proxy = get_class_property_proxy(schema, "prop")
    new_proxy = get_class_property_proxy(new_schema, "prop")
    model = ErrorBoolModel()
    target = "karabogui.controllers.display.errorbool.SvgWidget"
    mocker.patch(target, new=MockQSvgWidget)
    proxy.value = False

    # basics
    controller = DisplayErrorBool(proxy=proxy, model=model)
    controller.create(None)
    assert controller.widget is not None

    # exercise code paths
    apply_configuration(Hash("prop", True), proxy.root_proxy.binding)
    assert controller.widget.loaded_data is OK_BOOL
    apply_configuration(Hash("prop", False), proxy.root_proxy.binding)
    assert controller.widget.loaded_data is ERROR_BOOL
    controller.add_proxy(new_proxy)
    assert controller.widget.loaded_data is ERROR_BOOL
    apply_configuration(Hash("prop", True), proxy.root_proxy.binding)
    assert controller.widget.loaded_data is OK_BOOL

    # teardown
    controller.destroy()
    assert controller.widget is None
