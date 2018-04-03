from unittest.mock import Mock, patch

from karabo.common.enums import DeviceStatus
from karabo.middlelayer import Configurable, VectorString
from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, SceneTargetWindow)
from karabogui.controllers.display.devicescenelink import LinkWidget
from karabogui.sceneview.widget.container import ControllerContainer
from karabogui.singletons.api import get_topology
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy)
from ..devicescenelink import DisplayDeviceSceneLink


class Object(Configurable):
    availableScenes = VectorString(defaultValue=['bob', 'frank'])


class TestDisplayDeviceSceneLink(GuiTestCase):
    def setUp(self):
        super(TestDisplayDeviceSceneLink, self).setUp()

        self.model = DeviceSceneLinkModel()
        self.model.keys = ['deviceUno.availableScenes']
        self.model.target_window = SceneTargetWindow.Dialog

    def test_clicked_device_off(self):

        container = ControllerContainer(klass=DisplayDeviceSceneLink,
                                        model=self.model,
                                        parent=None)

        container._device_status_changed(DeviceStatus.OFFLINE)
        self.assertFalse(container.widget_controller.widget.isEnabled())

        container._device_status_changed(DeviceStatus.ONLINE)
        self.assertTrue(container.widget_controller.widget.isEnabled())

        container._device_status_changed(DeviceStatus.OFFLINE)
        self.assertFalse(container.widget_controller.widget.isEnabled())
