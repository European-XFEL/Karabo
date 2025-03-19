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
from numpy import uint64

from karabo.common.api import State
from karabo.common.scenemodel.api import SingleBitModel
from karabo.native import Configurable, Int32
from karabogui.indicators import STATE_COLORS
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..singlebit import SingleBit

COLORS = {s: str(STATE_COLORS[s]) for s in (State.ACTIVE, State.PASSIVE)}


class Object(Configurable):
    prop = Int32(defaultValue=0)


@pytest.fixture
def singlebit_setup(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    model = SingleBitModel()
    controller = SingleBit(proxy=proxy, model=model)
    controller.create(None)
    assert controller.widget is not None
    yield controller, proxy, model

    controller.destroy()
    assert controller.widget is None


def test_set_value(singlebit_setup):
    controller, proxy, model = singlebit_setup
    singlebit = controller._internal_widget

    model.invert = False
    model.bit = 5

    set_proxy_value(proxy, "prop", uint64(32))
    assert COLORS[State.ACTIVE] in singlebit.styleSheet()

    set_proxy_value(proxy, "prop", uint64(31))
    assert COLORS[State.PASSIVE] in singlebit.styleSheet()

    model.bit = 4
    assert COLORS[State.ACTIVE] in singlebit.styleSheet()

    model.invert = True
    assert COLORS[State.PASSIVE] in singlebit.styleSheet()


def test_read_only(singlebit_setup):
    controller, _, _ = singlebit_setup
    singlebit = controller._internal_widget

    controller.set_read_only(True)
    assert not singlebit.isEnabled()

    controller.set_read_only(False)
    assert singlebit.isEnabled()


def test_actions(singlebit_setup, mocker):
    controller, proxy, model = singlebit_setup
    actions = controller.widget.actions()
    invert_action = actions[0]
    change_action = actions[1]
    assert "invert" in invert_action.text().lower()
    assert "change" in change_action.text().lower()

    model.invert = False
    invert_action.trigger()
    assert model.invert

    sym = "karabogui.controllers.display.singlebit.QInputDialog"
    QInputDialog = mocker.patch(sym)
    QInputDialog.getInt.return_value = 3, True
    change_action.trigger()
    assert model.bit == 3

    proxy.binding.displayType = "bin|1,2,3"
    QInputDialog.getItem.return_value = "2:", True
    change_action.trigger()

    assert model.bit == 2
