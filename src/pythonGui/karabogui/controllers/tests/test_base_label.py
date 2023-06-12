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
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import DisplayFloatModel
from karabo.native import Configurable, Float, String
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, ProxyStatus, build_binding)
from karabogui.testing import set_proxy_value

from ..baselabel import BaseLabelController, FormatMixin


class Object(Configurable):
    string = String()
    decimals = Float(displayType="format|fmt=f&decimals=6")


def test_base_label_controller(gui_app, mocker):
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceClassProxy(binding=binding, server_id="Fake",
                              status=ProxyStatus.OFFLINE)
    string = PropertyProxy(root_proxy=device, path="string")
    decimals = PropertyProxy(root_proxy=device, path="decimals")

    model = DisplayFloatModel()
    controller = BaseLabelController(model=model, proxy=string)
    controller.create(None)
    assert controller.widget is not None
    set_proxy_value(string, "string", "hello")
    assert controller.internal_widget.text() == "hello"

    controller = BaseLabelController(model=model, proxy=decimals)
    controller.create(None)
    set_proxy_value(decimals, "decimals", 0.25)
    assert controller.internal_widget.text() == "0.25"

    action = controller.widget.actions()[0]
    assert action.text() == "Format field"
    dialog = "karabogui.controllers.baselabel.FormatLabelDialog"
    dialog = mocker.patch(dialog)
    dialog().font_size = 9
    dialog().font_weight = "bold"
    dialog().exec.return_value = QDialog.Accepted
    action.trigger()
    assert model.font_weight == "bold"
    assert model.font_size == 9


class BaseFloatController(FormatMixin, BaseLabelController):
    """A class to test the base format mixin"""


def test_format_mixin(gui_app, mocker):
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceClassProxy(binding=binding, server_id="Fake",
                              status=ProxyStatus.OFFLINE)
    decimals = PropertyProxy(root_proxy=device, path="decimals")

    model = DisplayFloatModel()
    controller = BaseFloatController(model=model, proxy=decimals)
    controller.create(None)
    assert controller.widget is not None
    set_proxy_value(decimals, "decimals", 10.25)
    # Default format is "{.8g}"
    assert controller.fmt == "{:.8g}"
    assert controller.internal_widget.text() == "10.25"

    # Format field, already tested
    action = controller.widget.actions()[0]
    assert action.text() == "Format field"

    # Formatting value dialog
    action = controller.widget.actions()[1]
    assert action.text() == "Format value"
    dialog = "karabogui.controllers.baselabel.FormatFmtDialog"
    dialog = mocker.patch(dialog)
    dialog().fmt = "f"
    dialog().decimals = "4"
    dialog().exec.return_value = QDialog.Accepted
    action.trigger()
    assert model.fmt == "f"
    assert model.decimals == "4"

    assert controller.fmt == "{:.4f}"
    assert controller.internal_widget.text() == "10.2500"

    # Check the drop of the float controller
    BaseFloatController.initialize_model(decimals, model)
    assert model.decimals == "6"
    assert model.fmt == "f"
