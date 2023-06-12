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

from karabo.common.scenemodel.api import DisplayProgressBarModel
from karabo.native import Configurable, Float, Int8
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..progressbar import NULL_RANGE, PROGRESS_MAX, DisplayProgressBar


class Object(Configurable):
    prop = Float(minInc=-2.0, maxInc=4.0)


class ObjectWithoutLimits(Configurable):
    prop = Int8()


@pytest.fixture
def progressbar_setup(gui_app):
    schema = Object.getClassSchema()
    model = DisplayProgressBarModel(is_vertical=True)
    proxy = get_class_property_proxy(schema, "prop")
    controller = DisplayProgressBar(proxy=proxy, model=model)
    controller.create(None)
    yield controller, proxy, model

    # teardown
    controller.destroy()
    assert controller.widget is None


def test_widget(progressbar_setup):
    controller, _, _ = progressbar_setup
    assert controller.widget.minimum() == 0
    assert controller.widget.maximum() == PROGRESS_MAX


def test_orientation(progressbar_setup):
    controller, _, _ = progressbar_setup
    assert controller.widget.orientation() == Qt.Vertical

    # Trigger the orientation changing action
    action = controller.widget.actions()[0]
    assert action.text() == "Change Orientation"

    action.trigger()
    assert controller.widget.orientation() == Qt.Horizontal


def test_set_value(progressbar_setup):
    controller, proxy, _ = progressbar_setup
    # value range is [-2, 4]; 1.0 is the middle
    set_proxy_value(proxy, "prop", 1.0)
    assert controller.widget.value() == PROGRESS_MAX * 0.5


def test_no_limits_messagebox(gui_app):
    schema = ObjectWithoutLimits.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    model = DisplayProgressBarModel(is_vertical=True)
    controller = DisplayProgressBar(proxy=proxy, model=model)
    controller.create(None)

    assert controller.widget.minimum() == 0
    assert controller.widget.maximum() == 0
    assert controller._value_factors == NULL_RANGE

    # tear down
    controller.destroy()
    assert controller.widget is None
