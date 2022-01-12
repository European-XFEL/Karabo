from pathlib import Path
from unittest import mock

from qtpy.QtCore import QEvent, QPoint, Qt
from qtpy.QtGui import QMouseEvent

from karabo.common.scenemodel.api import ImageRendererModel, SceneModel
from karabogui import icons
from karabogui.sceneview.api import SceneView
from karabogui.testing import GuiTestCase

from ..drawing import ImageRendererTool


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
