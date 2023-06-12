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
from pathlib import Path
from unittest import mock

from qtpy.QtCore import QEvent, QPoint, Qt
from qtpy.QtGui import QMouseEvent
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, ImageRendererModel, InstanceStatusModel, SceneModel)
from karabogui import icons
from karabogui.sceneview.api import SceneView
from karabogui.testing import GuiTestCase

from ..drawing import (
    DeviceSceneLinkTool, ImageRendererTool, InstanceStatusTool)


class TestDrawingTools(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.scene_model = SceneModel()
        self.scene_view = SceneView(model=self.scene_model)

    def tearDown(self):
        super().tearDown()
        self.scene_view.destroy()
        self.scene_view = None
        self.scene_model = None

    def test_svg_renderer_tool(self):
        self.assertEqual(len(self.scene_model.children), 0)
        dialog_path = "karabogui.sceneview.tools.drawing.getOpenFileName"
        with mock.patch(dialog_path) as dia:
            path = str(Path(icons.__file__).parent / "about.svg")
            dia.return_value = path

            tool = ImageRendererTool()

            event = QMouseEvent(
                QEvent.MouseMove,
                QPoint(20, 0),
                Qt.LeftButton,
                Qt.LeftButton,
                Qt.NoModifier)

            tool.mouse_up(self.scene_view, event)

            self.assertEqual(len(self.scene_model.children), 1)
            model = self.scene_model.children[0]
            self.assertIsInstance(model, ImageRendererModel)
            self.assertEqual(model.x, 20)
            self.assertEqual(model.y, 0)

    def test_device_scene_link(self):
        self.assertEqual(len(self.scene_model.children), 0)
        path = "karabogui.sceneview.tools.drawing.DeviceCapabilityDialog"
        with mock.patch(path) as dia:
            dia().device_id = "TestDevice"
            dia().capa_name = "scene"
            dia().exec.return_value = QDialog.Accepted

            tool = DeviceSceneLinkTool()

            event = QMouseEvent(
                QEvent.MouseMove,
                QPoint(20, 0),
                Qt.LeftButton,
                Qt.LeftButton,
                Qt.NoModifier)

            tool.mouse_up(self.scene_view, event)

            self.assertEqual(len(self.scene_model.children), 1)
            model = self.scene_model.children[0]
            self.assertIsInstance(model, DeviceSceneLinkModel)
            self.assertEqual(model.x, 20)
            self.assertEqual(model.y, 0)
            self.assertEqual(model.target, "scene")
            self.assertEqual(model.keys, ["TestDevice.availableScenes"])

    def test_instance_status(self):
        self.assertEqual(len(self.scene_model.children), 0)
        path = "karabogui.sceneview.tools.drawing.TopologyDeviceDialog"
        with mock.patch(path) as dia:
            dia().device_id = "TestDevice"
            dia().exec.return_value = QDialog.Accepted

            tool = InstanceStatusTool()

            event = QMouseEvent(
                QEvent.MouseMove,
                QPoint(20, 0),
                Qt.LeftButton,
                Qt.LeftButton,
                Qt.NoModifier)

            tool.mouse_up(self.scene_view, event)

            self.assertEqual(len(self.scene_model.children), 1)
            model = self.scene_model.children[0]
            self.assertIsInstance(model, InstanceStatusModel)
            self.assertEqual(model.x, 20)
            self.assertEqual(model.y, 0)
            self.assertEqual(model.height, 30)
            self.assertEqual(model.width, 30)
            self.assertEqual(model.keys, ["TestDevice.deviceId"])
