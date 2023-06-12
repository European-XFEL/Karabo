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

from karabo.common.scenemodel.api import EditableSpinBoxModel
from karabo.native import Configurable, Int32, UInt8, Unit
from karabogui.binding.api import build_binding
from karabogui.const import IS_MAC_SYSTEM
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..intspinbox import EditableSpinBox


class Object(Configurable):
    prop = Int32(minInc=-10, maxInc=10, unitSymbol=Unit.SECOND)


class Other(Configurable):
    prop = UInt8()


@pytest.fixture
def intspinbox_setup(gui_app):
    # teardown
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    controller = EditableSpinBox(proxy=proxy, model=EditableSpinBoxModel())
    controller.create(None)
    assert controller.widget is not None
    controller.set_read_only(False)
    yield controller, proxy
    # teardown
    controller.destroy()
    assert controller.widget is None


def test_intspinbox_basics(intspinbox_setup):
    controller, proxy = intspinbox_setup
    # set value
    set_proxy_value(proxy, "prop", 5)
    assert controller.widget.value() == 5

    # edit value
    controller.widget.setValue(3)
    assert proxy.edit_value == 3

    # focus policy
    assert controller.widget.focusPolicy() == Qt.StrongFocus


def test_intspinbox_schema_update(gui_app):
    proxy = get_class_property_proxy(Other.getClassSchema(), "prop")
    controller = EditableSpinBox(proxy=proxy)
    controller.create(None)

    assert controller.widget.minimum() == 0
    assert controller.widget.maximum() == 255
    assert controller.widget.suffix() == ""

    build_binding(Object.getClassSchema(),
                  existing=proxy.root_proxy.binding)

    assert controller.widget.minimum() == -10
    assert controller.widget.maximum() == 10
    assert controller.widget.suffix() == " s"


@pytest.mark.skipif(IS_MAC_SYSTEM, reason="Fonts are different on MacOS")
def test_intspinbox_font_setting(intspinbox_setup, mocker):
    controller, _ = intspinbox_setup
    settings = "{ font: normal; font-size: 10pt; }"
    assert settings in controller.widget.styleSheet()

    path = "karabogui.controllers.edit.intspinbox.FormatLabelDialog"
    dialog = mocker.patch(path)
    dialog().exec.return_value = QDialog.Accepted
    dialog().font_size = 11
    dialog().font_weight = "bold"
    controller._format_field()

    settings = "{ font: bold; font-size: 11pt; }"
    assert settings in controller.widget.styleSheet()
