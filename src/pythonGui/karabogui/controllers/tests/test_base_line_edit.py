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
from traits.api import Instance, Int

from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabo.native import AccessMode, Configurable, String
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, ProxyStatus, StringBinding, build_binding)
from karabogui.testing import flushed_registry, set_proxy_value

from ..baselineedit import BaseLineEditController
from ..registry import register_binding_controller


class Object(Configurable):
    string = String(accessMode=AccessMode.RECONFIGURABLE)


class UniqueWidgetModel(BaseWidgetObjectData):
    pass


def _define_binding_classes():
    @register_binding_controller(klassname="NewEdit",
                                 binding_type=StringBinding,
                                 can_edit=True, priority=-30)
    class CustomLineEdit(BaseLineEditController):
        model = Instance(UniqueWidgetModel, args=())

        counter = Int(0)

        def onText(self, text):
            super().onText(text)
            self.counter += 1

    return {
        "CustomLineEdit": CustomLineEdit,
    }


def test_custom_line_edit(gui_app):
    """Test the creation of a simple `BaseLineEditController`"""
    with flushed_registry():
        klasses = _define_binding_classes()

    # Save the classes so they only need to be declared once
    edit_controller = klasses["CustomLineEdit"]

    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceClassProxy(binding=binding, server_id="NoDeviceServer",
                              status=ProxyStatus.OFFLINE)
    proxy = PropertyProxy(root_proxy=device, path="string")
    assert proxy.binding is not None

    controller = edit_controller(proxy=proxy)
    controller.create(None)
    controller.set_read_only(False)
    assert controller.widget is not None
    assert controller.counter == 0

    controller.finish_initialization()
    assert controller.internal_widget.validator() is None
    set_proxy_value(proxy, "string", "Update")
    assert controller.internal_widget.text() == "Update"
    assert controller.counter == 0

    controller.internal_widget.setText("NoBeam")
    assert controller.counter == 1
    assert controller.internal_widget.text() == "NoBeam"

    assert controller.internal_widget.isEnabled()
    controller.setEnabled(False)
    assert not controller.internal_widget.isEnabled()
    controller.setEnabled(True)
    assert controller.internal_widget.isEnabled()
