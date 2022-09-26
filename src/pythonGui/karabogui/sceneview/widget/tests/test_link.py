from unittest import main, mock

from qtpy.QtCore import QPoint, QRect
from qtpy.QtWidgets import QDialog, QWidget

from karabo.common.scenemodel.api import (
    LabelModel, SceneLinkModel, SceneTargetWindow, WebLinkModel)
from karabogui.events import KaraboEvent
from karabogui.testing import GuiTestCase

from ..link import SceneLinkWidget, WebDialog, WebLinkWidget


class TestSceneLink(GuiTestCase):

    def setUp(self):
        super().setUp()
        false_font = "Times New Roman,10,-1,5,50,0,0,0,0,0"
        self.model = SceneLinkModel(
            text="SceneLink Text", foreground='black',
            x=0, y=0, width=100, height=100, font=false_font,
            target="name:122345677")

        self.main_widget = QWidget()
        self.widget = SceneLinkWidget(model=self.model,
                                      parent=self.main_widget)
        self.main_widget.show()

    def tearDown(self):
        super().tearDown()
        self.main_widget = None
        self.widget = None

    # -----------------------------------------------------------------------
    # Actual tests

    def test_font_replacement(self):
        font = self.widget.model.font
        self.assertEqual(font, "Source Sans Pro,10,-1,5,50,0,0,0,0,0")

    def test_basics(self):
        model_rect = QRect(self.model.x, self.model.y,
                           self.model.width, self.model.height, )
        self.assertTrue(self.widget.rect() == model_rect)
        self.assertEqual(len(self.widget.actions()), 2)
        target = 'karabogui.sceneview.widget.link.broadcast_event'
        with mock.patch(target) as broadcast_event:
            self.click(self.widget)
            self.process_qt_events()
            broadcast_event.assert_called_with(
                KaraboEvent.OpenSceneLink,
                {'name': 'name', 'target': '122345677',
                 'target_window': SceneTargetWindow.MainWindow})

        # Make sure, basic interface is there
        self.widget.add_proxies(None)
        self.widget.apply_changes()
        self.widget.decline_changes()
        self.widget.destroy()
        self.widget.set_visible(True)
        self.widget.update_global_access_level(None)
        self.assertIsNotNone(self.widget)

        # Geometry
        rect = QRect(2, 10, 2, 10)
        self.assertFalse(self.widget.geometry() == rect)
        self.widget.set_geometry(rect)
        self.assertTrue(self.widget.geometry() == rect)

        # Translation
        self.assertEqual(self.widget.pos(), QPoint(2, 10))
        self.widget.translate(QPoint(10, 0))
        self.assertEqual(self.widget.pos(), QPoint(12, 10))

        path = "karabogui.sceneview.widget.link.TextDialog"
        with mock.patch(path) as dialog:
            dialog.exec.return_value = QDialog.Accepted
            dialog().label_model = LabelModel(text="This is a label text")
            self.widget.edit_label()
            self.assertEqual(self.model.text, "This is a label text")

        path = "karabogui.sceneview.widget.link.SceneLinkDialog"
        with mock.patch(path) as dialog:
            dialog.exec.return_value = QDialog.Accepted
            dialog().selectedScene = "New selected scene"
            dialog().selectedTargetWindow = SceneTargetWindow.Dialog
            self.widget.edit()
            self.assertEqual(self.model.target, "New selected scene")
            self.assertEqual(self.model.target_window,
                             SceneTargetWindow.Dialog)


class TestWeblink(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.model = WebLinkModel(
            text="SceneLink Text", foreground='black',
            x=0, y=0, width=100, height=100,
            target="www.xfel.eu")

        self.main_widget = QWidget()
        self.widget = WebLinkWidget(model=self.model,
                                    parent=self.main_widget)
        self.main_widget.show()

    def tearDown(self):
        super().tearDown()
        self.main_widget = None
        self.widget = None

    # -----------------------------------------------------------------------
    # Actual tests

    def test_basics(self):
        model_rect = QRect(self.model.x, self.model.y,
                           self.model.width, self.model.height, )
        self.assertTrue(self.widget.rect() == model_rect)
        self.assertEqual(len(self.widget.actions()), 2)
        target = 'karabogui.sceneview.widget.link.webbrowser'
        with mock.patch(target) as browser:
            self.click(self.widget)
            self.process_qt_events()
            browser.open_new.assert_called_with("www.xfel.eu")

        # Make sure, basic interface is there
        self.widget.add_proxies(None)
        self.widget.apply_changes()
        self.widget.decline_changes()
        self.widget.destroy()
        self.widget.set_visible(True)
        self.widget.update_global_access_level(None)
        self.assertIsNotNone(self.widget)

        # Geometry
        rect = QRect(2, 10, 2, 10)
        self.assertFalse(self.widget.geometry() == rect)
        self.widget.set_geometry(rect)
        self.assertTrue(self.widget.geometry() == rect)

        # Translation
        self.assertEqual(self.widget.pos(), QPoint(2, 10))
        self.widget.translate(QPoint(10, 0))
        self.assertEqual(self.widget.pos(), QPoint(12, 10))

        path = "karabogui.sceneview.widget.link.TextDialog"
        with mock.patch(path) as dialog:
            dialog.exec.return_value = QDialog.Accepted
            dialog().label_model = LabelModel(text="This is a label text")
            self.widget.edit_label()
            self.assertEqual(self.model.text, "This is a label text")

        path = "karabogui.sceneview.widget.link.WebDialog"
        with mock.patch(path) as dialog:
            dialog.exec.return_value = QDialog.Accepted
            dialog().target = "www.desy.de"
            self.widget.edit()
            self.assertEqual(self.model.target, "www.desy.de")

    def test_webdialog(self):
        dialog = WebDialog(self.model.target)
        self.assertNotEqual(dialog.target, self.model.target)
        # Dialog corrects with http
        self.assertEqual(dialog.target, "http://www.xfel.eu")


if __name__ == "__main__":
    main()
