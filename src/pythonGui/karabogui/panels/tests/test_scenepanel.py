from unittest import mock

from qtpy.QtCore import QSize
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import SceneModel
from karabogui.singletons.mediator import Mediator
from karabogui.testing import GuiTestCase, singletons

from ..scenepanel import ScenePanel

DIALOG_PATH = "karabogui.panels.scenepanel.ResizeSceneDialog"


class TestScenePanel(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.model = SceneModel()
        self.mediator = Mediator()
        # Use own mediator as closure will send signals
        with singletons(mediator=self.mediator):
            self.panel = ScenePanel(self.model, connected_to_server=True)
            container = mock.MagicMock()
            container.maximized.new = False
            self.panel.attach_to_container(container)
            self.panel.show()

    def tearDown(self):
        super().tearDown()
        with singletons(mediator=self.mediator):
            self.panel.close()
            self.panel.destroy()
            self.model = None
            self.panel = None
        self.mediator = None

    def test_resize_scene(self):
        panel_size = self.panel.size()
        self.assertEqual(self.scene_size, self.model_size)

        # Docked
        width, height = (100, 200)
        self._resize_scene(width, height, docked=True)
        self.assertTrue(self.panel.is_docked)
        self.assertEqual(self.scene_size, (width, height))
        self.assertEqual(self.scene_size, self.model_size)
        self.assertEqual(self.panel.size(), panel_size)

        # Undocked
        width, height = (300, 400)
        self.panel.onUndock()
        self._resize_scene(width, height, docked=False)
        self.assertFalse(self.panel.is_docked)
        self.assertEqual(self.scene_size, (width, height))
        self.assertEqual(self.scene_size, self.model_size)
        self.assertNotEqual(self.panel.size(), panel_size)

    def _resize_scene(self, width=100, height=200, docked=False):
        mocked_exec = mock.patch(f"{DIALOG_PATH}.exec",
                                 return_value=QDialog.Accepted)
        mocked_size = mock.patch(f"{DIALOG_PATH}.scene_size",
                                 new=QSize(width, height))
        with mocked_exec, mocked_size:
            self.panel.design_mode_changed(True)
            self.panel.is_docked = docked
            self.panel.resize_scene()

    @property
    def scene_size(self):
        size = self.panel.scene_view.size()
        return size.width(), size.height()

    @property
    def model_size(self):
        return self.model.width, self.model.height
