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
import pytest
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import FloatSpinBoxModel
from karabo.native import Configurable, Double, Unit
from karabogui.binding.api import build_binding
from karabogui.const import IS_MAC_SYSTEM
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..floatspinbox import FloatSpinBox


class Object(Configurable):
    prop = Double(minInc=-10.0, maxInc=10.0, absoluteError=0.5,
                  unitSymbol=Unit.METER)


class Other(Configurable):
    prop = Double(minInc=1.0, maxInc=42.0)


@pytest.fixture
def floatspinbox_setup(gui_app):
    # setup
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    model = FloatSpinBoxModel(step=0.1, decimals=5)
    controller = FloatSpinBox(proxy=proxy, model=model)
    controller.create(None)
    assert controller.widget is not None
    controller.set_read_only(False)
    yield controller, proxy
    # teardown
    controller.destroy()
    assert controller.widget is None


def test_floatspinbox_basics(floatspinbox_setup):
    controller, proxy = floatspinbox_setup
    # focus policy
    assert controller.widget.focusPolicy() == Qt.StrongFocus

    # set value
    set_proxy_value(proxy, "prop", 5.0)
    assert controller.widget.value() == 5.0

    # edit value
    controller.widget.setValue(3.0)
    assert proxy.edit_value == 3.0


def test_floatspinbox_schema_update(gui_app):
    proxy = get_class_property_proxy(Other.getClassSchema(), "prop")
    controller = FloatSpinBox(proxy=proxy, model=FloatSpinBoxModel())
    controller.create(None)
    assert controller.widget.suffix() == ""

    assert controller.widget.minimum() == 1.0
    assert controller.widget.maximum() == 42.0

    build_binding(Object.getClassSchema(),
                  existing=proxy.root_proxy.binding)

    assert controller.widget.minimum() == -10.0
    assert controller.widget.maximum() == 10.0
    assert controller.widget.suffix() == " m"


def test_floatspinbox_actions(floatspinbox_setup, mocker):
    controller, _ = floatspinbox_setup
    actions = controller.widget.actions()
    change_step, change_decimals, formatting = actions

    assert "step" in change_step.text().lower()
    assert "decimals" in change_decimals.text().lower()

    sym = "karabogui.controllers.edit.floatspinbox.QInputDialog"
    QInputDialog = mocker.patch(sym)
    QInputDialog.getDouble.return_value = 0.25, True
    QInputDialog.getInt.return_value = 9, True

    change_step.trigger()
    assert controller.model.step == 0.25

    change_decimals.trigger()
    assert controller.model.decimals == 9


@pytest.mark.skipif(IS_MAC_SYSTEM, reason="Fonts are different on MacOS")
def test_floatspinbox_font_setting(floatspinbox_setup, mocker):
    controller, _ = floatspinbox_setup
    settings = "{ font: normal; font-size: 10pt; }"
    assert settings in controller.widget.styleSheet()

    path = "karabogui.controllers.edit.floatspinbox.FormatLabelDialog"
    dialog = mocker.patch(path)
    dialog().exec.return_value = QDialog.Accepted
    dialog().font_size = 11
    dialog().font_weight = "bold"
    controller._format_field()

    settings = "{ font: bold; font-size: 11pt; }"
    assert settings in controller.widget.styleSheet()
