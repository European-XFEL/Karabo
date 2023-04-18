from qtpy.QtWidgets import QLabel

from karabo.native import Configurable, String
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..lamp import LampWidget


class MockLabel(QLabel):
    def setPixmap(self, pixmap):
        self.pixmap = pixmap

    def setMaximumWidth(self, value):
        self.width = value

    def setMaximumHeight(self, value):
        self.height = value


class Object(Configurable):
    state = String()


def test_set_values(gui_app, mocker):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "state")
    target = "karabogui.controllers.display.lamp.QLabel"
    mocker.patch(target, new=MockLabel)

    states = ("CHANGING", "ACTIVE", "PASSIVE", "DISABLED", "STATIC",
              "RUNNING", "NORMAL", "ERROR", "INIT", "UNKNOWN")

    controller = LampWidget(proxy=proxy)
    controller.create(None)

    for state in states:
        set_proxy_value(proxy, "state", state)
        assert controller.widget.pixmap is not None
        controller.widget.pixmap = None
