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
import re

from karabo.common.scenemodel.api import HistoricTextModel
from karabo.native import (
    Configurable, Hash, String, Timestamp, UInt32, VectorString)
from karabogui.const import IS_WINDOWS_SYSTEM
from karabogui.controllers.display.historic_text import DisplayHistoricText
from karabogui.testing import (
    click_button, get_property_proxy, set_proxy_value, singletons)

# Allows for one or two digits for day, month, hour, minute, second
# and exactly three digits for milliseconds
pattern = r"\[\d{2}-\d{2}-\d{4} \d{2}:\d{2}:\d{2}\.\d{3}\]"


class Object(Configurable):
    prop = String()
    vector = VectorString()
    integer = UInt32(
        displayType="hex|",
        defaultValue=13
    )


def test_historic_text(gui_app, subtests, mocker):
    """Test the historic text widget with string data"""
    schema = Object.getClassSchema()
    proxy = get_property_proxy(schema, "prop")
    controller = DisplayHistoricText(proxy=proxy,
                                     model=HistoricTextModel())
    controller.create(None)
    assert controller.widget is not None

    if not IS_WINDOWS_SYSTEM:
        # Note: PlainText not working on windows
        model = controller.list_model
        model.setStringList([])
        assert model.rowCount() == 0
        set_proxy_value(proxy, "prop", "Line 1")
        assert "Line 1" in controller.text_widget.toPlainText()
        set_proxy_value(proxy, "prop", "Line 2")
        assert re.search(pattern, controller.text_widget.toPlainText())
        assert "Line 2" in controller.text_widget.toPlainText()

    with subtests.test("Historic data arrival"):
        xfel = Hash("v", "xfel")
        ts_xfel = Timestamp()
        ts_xfel.toHashAttributes(xfel)
        karabo = Hash("v", "karabo")
        ts_karabo = Timestamp()
        ts_karabo.toHashAttributes(karabo)
        controller._historic_data_arrival([xfel, karabo])
        model = controller.list_model
        assert model.rowCount() == 2
        index = model.index(0, 0)
        assert re.search(pattern, index.data())

    with subtests.test("Historic data request"):
        network = mocker.Mock()
        with singletons(network=network):
            click_button(controller.request_button)
            text = controller.status_widget.text()
            assert "Requesting historic data!" in text
            network.onGetPropertyHistory.assert_called_once()

    controller.destroy()
    assert controller.widget is None


def test_vector_historic(gui_app, subtests, mocker):
    """Test the historic text widget with vector string data"""
    schema = Object.getClassSchema()
    proxy = get_property_proxy(schema, "vector")
    controller = DisplayHistoricText(proxy=proxy,
                                     model=HistoricTextModel())
    controller.create(None)
    assert controller.widget is not None

    if not IS_WINDOWS_SYSTEM:
        # Note: PlainText not working on windows
        model = controller.list_model
        model.setStringList([])
        assert model.rowCount() == 0
        set_proxy_value(proxy, "vector", ["one", "two"])
        assert "['one', 'two']" in controller.text_widget.toPlainText()
        set_proxy_value(proxy, "vector", ["one", "three", "five"])
        text = controller.text_widget.toPlainText()
        assert re.search(pattern, controller.text_widget.toPlainText())
        assert "['one', 'three', 'five']" in text

    with subtests.test("Historic data arrival"):
        first = Hash("v", ["a1", "b2", "c3"])
        ts_first = Timestamp()
        ts_first.toHashAttributes(first)
        second = Hash("v", ["one only"])
        ts_second = Timestamp()
        ts_second.toHashAttributes(second)
        third = Hash("v", [])
        ts_third = Timestamp()
        ts_third.toHashAttributes(third)
        controller._historic_data_arrival([first, second, third])
        model = controller.list_model
        assert model.rowCount() == 3
        index = model.index(0, 0)
        assert re.search(pattern, index.data())

    with subtests.test("Historic data request"):
        network = mocker.Mock()
        with singletons(network=network):
            click_button(controller.request_button)
            text = controller.status_widget.text()
            assert "Requesting historic data!" in text
            network.onGetPropertyHistory.assert_called_once()

    controller.destroy()
    assert controller.widget is None


def test_integer_hex(gui_app, subtests, mocker):
    """Test the historic text widget with vector string data"""
    schema = Object.getClassSchema()
    proxy = get_property_proxy(schema, "integer")
    controller = DisplayHistoricText(proxy=proxy,
                                     model=HistoricTextModel())
    controller.create(None)
    assert controller.widget is not None

    if not IS_WINDOWS_SYSTEM:
        # Note: PlainText not working on windows
        model = controller.list_model
        model.setStringList([])
        assert model.rowCount() == 0
        set_proxy_value(proxy, "integer", 10)
        assert "0xA" in controller.text_widget.toPlainText()
        set_proxy_value(proxy, "integer", 7)
        text = controller.text_widget.toPlainText()
        assert "0x7" in text

    with subtests.test("Historic data arrival"):
        first = Hash("v", 2)
        ts_first = Timestamp()
        ts_first.toHashAttributes(first)
        second = Hash("v", 1)
        ts_second = Timestamp()
        ts_second.toHashAttributes(second)
        third = Hash("v", 0)
        ts_third = Timestamp()
        ts_third.toHashAttributes(third)
        controller._historic_data_arrival([first, second, third])
        model = controller.list_model
        assert model.rowCount() == 3

    with subtests.test("Historic data request"):
        network = mocker.Mock()
        with singletons(network=network):
            click_button(controller.request_button)
            text = controller.status_widget.text()
            assert "Requesting historic data!" in text
            network.onGetPropertyHistory.assert_called_once()

    controller.destroy()
    assert controller.widget is None
