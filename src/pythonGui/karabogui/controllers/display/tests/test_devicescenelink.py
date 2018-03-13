from unittest.mock import Mock, patch

from karabo.middlelayer import Configurable, VectorString
from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, SceneTargetWindow)
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy)
from ..devicescenelink import DisplayDeviceSceneLink


class Object(Configurable):
    availableScenes = VectorString(defaultValue=['bob', 'frank'])


class TestDisplayDeviceSceneLink(GuiTestCase):
    def setUp(self):
        super(TestDisplayDeviceSceneLink, self).setUp()
        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'availableScenes')
        self.proxy.value = ['bob', 'frank']
        self.controller = DisplayDeviceSceneLink(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_clicked(self):
        self.controller.model = DeviceSceneLinkModel()
        self.controller.model.keys = ['deviceUno.availableScenes']
        self.controller.model.target = 'Vinny'
        self.controller.model.target_window = SceneTargetWindow.Dialog
        target = 'karabogui.controllers.display.' +\
                 'devicescenelink.call_device_slot'
        caller = Mock()
        with patch(target) as caller:
            self.controller._internal_widget._handle_click()
            assert caller.call_count == 1
