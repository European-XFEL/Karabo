from unittest.mock import patch

from qtpy.QtWidgets import QMessageBox

from karabo.common.enums import ProxyStatus
from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, SceneTargetWindow)
from karabo.native import Configurable, VectorString
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, singletons, system_hash)
from karabogui.topology.system_topology import SystemTopology

from ..devicescenelink import DisplayDeviceSceneLink


class MockBox(QMessageBox):

    @staticmethod
    def show_warning(text, title, parent=None):
        """Satisfy messagebox interface"""


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

        self.controller.model = DeviceSceneLinkModel()
        self.controller.model.keys = ['deviceUno.availableScenes']
        self.controller.model.target = 'Vinny'
        self.controller.model.target_window = SceneTargetWindow.Dialog
        self.controller.widget.model = self.controller.model
        self.target = 'karabogui.controllers.display.devicescenelink.' \
                      'get_scene_from_server'
        self.mbox = 'karabogui.controllers.display.devicescenelink.messagebox'

    def tearDown(self):
        super(TestDisplayDeviceSceneLink, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_clicked(self):
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(topology=topology):
            device = topology.get_device('deviceUno')
            device.status = ProxyStatus.ONLINE

            with patch(self.target) as caller:
                self.controller.widget._handle_click()
                assert caller.call_count == 1

    def test_clicked_device_off(self):
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(topology=topology):
            device = topology.get_device('deviceUno')
            device.status = ProxyStatus.OFFLINE

            with patch(self.target) as caller, patch(self.mbox, new=MockBox):
                self.controller.widget._handle_click()
                assert caller.call_count == 0
