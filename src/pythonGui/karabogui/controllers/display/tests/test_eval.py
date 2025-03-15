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
import numpy as np
import pytest

from karabo.common.scenemodel.api import EvaluatorModel
from karabo.native import Configurable, Float
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, ProxyStatus, build_binding)
from karabogui.controllers.display.eval import Evaluator
from karabogui.indicators import ALL_OK_COLOR
from karabogui.testing import set_proxy_value


class Object(Configurable):
    prop = Float()
    alarms = Float()


@pytest.fixture
def evaluator_setup(gui_app):
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceClassProxy(binding=binding, server_id="Fake",
                              status=ProxyStatus.OFFLINE)
    prop = PropertyProxy(root_proxy=device, path="prop")
    alarms = PropertyProxy(root_proxy=device, path="alarms")
    yield prop, alarms


def test_basics(evaluator_setup):
    prop, _ = evaluator_setup
    controller = Evaluator(proxy=prop, model=EvaluatorModel())
    controller.create(None)
    assert controller.widget is not None

    controller.destroy()
    assert controller.widget is None


def test_set_value(evaluator_setup):
    prop, _ = evaluator_setup
    model = EvaluatorModel(expression="x**2")
    controller = Evaluator(proxy=prop, model=model)
    controller.create(None)
    set_proxy_value(prop, "prop", 2.0)
    assert controller._internal_widget.text() == "4.0"


def test_builtin_function(evaluator_setup):
    prop, _ = evaluator_setup
    model = EvaluatorModel(expression="round(x)")
    controller = Evaluator(proxy=prop, model=model)
    controller.create(None)
    set_proxy_value(prop, "prop", 2.5)
    assert controller._internal_widget.text() == "2.0"


def test_change_expression(evaluator_setup, mocker):
    prop, _ = evaluator_setup
    set_proxy_value(prop, "prop", 21.0)
    controller = Evaluator(proxy=prop, model=EvaluatorModel())
    controller.create(None)
    action = controller.widget.actions()[0]
    assert action.text() == "Change expression..."

    dsym = "karabogui.controllers.display.eval.QInputDialog"
    QInputDialog = mocker.patch(dsym)
    QInputDialog.getText.return_value = "x*2", True
    action.trigger()
    assert controller._internal_widget.text() == "42.0"

    # Cause a SyntaxError messagebox
    msym = "karabogui.controllers.display.eval.messagebox"
    messagebox = mocker.patch(msym)
    QInputDialog.getText.return_value = "sin(x", True
    action.trigger()
    assert messagebox.show_warning.call_count == 1


def test_alarm_warning_fine_color(evaluator_setup):
    _, alarms = evaluator_setup
    controller = Evaluator(proxy=alarms)
    controller.create(None)

    set_proxy_value(alarms, "alarms", 0.75)
    assert controller._internal_widget.text() == "0.75"
    assert controller._bg_color == ALL_OK_COLOR

    set_proxy_value(alarms, "alarms", 3.0)
    assert controller._bg_color == ALL_OK_COLOR

    set_proxy_value(alarms, "alarms", 1.5)
    assert controller._bg_color == ALL_OK_COLOR


def test_type_conversion(evaluator_setup):
    prop, _ = evaluator_setup
    model = EvaluatorModel(expression="round(x,6)")
    controller = Evaluator(proxy=prop, model=model)
    controller.create(None)
    set_proxy_value(prop, "prop", np.float32(2.1234567899999))
    assert controller._internal_widget.text() == "2.123457"
