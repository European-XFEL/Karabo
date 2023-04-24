from platform import system

import pytest

from karabo.common.scenemodel.api import DisplayTextLogModel
from karabo.native import Configurable, String
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..textlog import DisplayTextLog


class Object(Configurable):
    prop = String()


@pytest.mark.skipif(system() == "Windows",
                    reason="toPlainText returns an empty string in windows")
def test_textlog_controller(gui_app):
    # setup
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = DisplayTextLog(proxy=proxy, model=DisplayTextLogModel())
    controller.create(None)
    assert controller.widget is not None
    # set value
    controller.log_widget.clear()
    set_proxy_value(proxy, "prop", "Line 1")
    assert "Line 1" in controller.log_widget.toPlainText()
    # teardown
    controller.destroy()
    assert controller.widget is None
