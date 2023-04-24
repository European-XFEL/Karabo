# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.common.scenemodel.api import HistoricTextModel
from karabo.native import Configurable, Hash, String, Timestamp, VectorString
from karabogui.const import IS_WINDOWS_SYSTEM
from karabogui.controllers.display.historic_text import DisplayHistoricText
from karabogui.testing import (
    click_button, get_property_proxy, set_proxy_value, singletons)


class Object(Configurable):
    prop = String()
    vector = VectorString()


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

    with subtests.test("Historic data request"):
        network = mocker.Mock()
        with singletons(network=network):
            click_button(controller.request_button)
            text = controller.status_widget.text()
            assert "Requesting historic data!" in text
            network.onGetPropertyHistory.assert_called_once()

    controller.destroy()
    assert controller.widget is None
