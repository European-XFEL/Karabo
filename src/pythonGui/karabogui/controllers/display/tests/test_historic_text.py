from platform import system
from unittest import main, mock, skipIf

from karabo.common.scenemodel.api import HistoricTextModel
from karabo.native import Configurable, Hash, String, Timestamp
from karabogui.controllers.display.historic_text import DisplayHistoricText
from karabogui.testing import (
    GuiTestCase, click_button, get_property_proxy, set_proxy_value, singletons)


class Object(Configurable):
    prop = String()


class TestHistoricText(GuiTestCase):
    def setUp(self):
        super().setUp()
        schema = Object.getClassSchema()
        self.proxy = get_property_proxy(schema, "prop")
        self.controller = DisplayHistoricText(proxy=self.proxy,
                                              model=HistoricTextModel())
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        super().tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    @skipIf(system() == "Windows",
            reason="On Windows the plain text is not working")
    def test_set_value(self):
        model = self.controller.list_model
        model.setStringList([])
        self.assertEqual(model.rowCount(), 0)
        set_proxy_value(self.proxy, "prop", "Line 1")
        self.assertIn("Line 1", self.controller.text_widget.toPlainText())
        set_proxy_value(self.proxy, "prop", "Line 2")
        self.assertIn("Line 2", self.controller.text_widget.toPlainText())

    def test_historic_data(self):
        xfel = Hash("v", "xfel")
        ts_xfel = Timestamp()
        ts_xfel.toHashAttributes(xfel)
        karabo = Hash("v", "karabo")
        ts_karabo = Timestamp()
        ts_karabo.toHashAttributes(karabo)
        self.controller._historic_data_arrival([xfel, karabo])
        model = self.controller.list_model
        self.assertEqual(model.rowCount(), 2)

    def test_historic_request(self):
        network = mock.Mock()
        with singletons(network=network):
            click_button(self.controller.request_button)
            text = self.controller.status_widget.text()
            self.assertIn("Requesting historic data!", text)
            network.onGetPropertyHistory.assert_called_once()


if __name__ == "__main__":
    main()
