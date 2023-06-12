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

from karabo.common.scenemodel.api import TickSliderModel
from karabogui.binding.api import build_binding
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..slider import TickSlider
from .utils import InRangeInt, LargeRange, Object, Other


@pytest.fixture
def slider_setup(gui_app):
    proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
    controller = TickSlider(proxy=proxy, model=TickSliderModel())
    controller.create(None)
    yield controller, proxy
    # teardown
    controller.destroy()
    assert controller.widget is None


def test_slider_focus_policy(slider_setup):
    controller, _ = slider_setup
    assert controller.slider.focusPolicy() == Qt.StrongFocus


def test_slider_set_value(slider_setup):
    controller, proxy = slider_setup
    set_proxy_value(proxy, 'prop', 1.0)
    assert controller.slider.value() == 1.0
    assert controller.label.text() == "1.0"
    set_proxy_value(proxy, 'prop', 1.3)
    assert controller.slider.value() == 1.0
    assert controller.label.text() == "1.3"
    set_proxy_value(proxy, 'prop', 1.9)
    assert controller.slider.value() == 1.0
    assert controller.label.text() == "1.9"


def test_slider_edit_value(slider_setup):
    controller, proxy = slider_setup
    controller.slider.valueChanged.emit(3)
    assert proxy.edit_value == 3
    assert controller.label.text() == "3.0"


def test_slider_schema_update(gui_app):
    proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
    controller = TickSlider(proxy=proxy)
    controller.create(None)

    assert controller.slider.minimum() == 1
    assert controller.slider.maximum() == 4

    build_binding(Object.getClassSchema(),
                  existing=proxy.root_proxy.binding)

    assert controller.slider.minimum() == -2.0
    assert controller.slider.maximum() == 4.0


def test_slider_actions(slider_setup, mocker):
    controller, proxy = slider_setup
    controller = TickSlider(proxy=proxy, model=TickSliderModel())
    controller.create(None)
    action = controller.widget.actions()[0]
    assert action.text() == 'Tick Interval'

    dsym = 'karabogui.controllers.edit.slider.QInputDialog'
    QInputDialog = mocker.patch(dsym)
    QInputDialog.getInt.return_value = 20, True
    action.trigger()
    assert controller.model.ticks == 20
    assert controller.slider.tickInterval() == 20
    assert controller.slider.singleStep() == 20

    action = controller.widget.actions()[1]
    assert action.text() == 'Show value'
    assert controller.model.show_value
    action.trigger()
    assert not controller.model.show_value

    controller.destroy()


def test_slider_large_range(gui_app):
    proxy = get_class_property_proxy(LargeRange.getClassSchema(), 'prop')
    controller = TickSlider(proxy=proxy)
    controller.create(None)

    assert not controller.widget.isEnabled()
    controller.destroy()


def test_slider_large_int(gui_app):
    proxy = get_class_property_proxy(InRangeInt.getClassSchema(), 'prop')
    controller = TickSlider(proxy=proxy)
    controller.create(None)

    assert controller.widget.isEnabled()
    controller.destroy()
