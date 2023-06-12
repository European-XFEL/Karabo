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
from unittest import main, mock

from qtpy.QtCore import QPoint, QRect
from qtpy.QtWidgets import QDialog, QWidget

from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, LabelModel, SceneLinkModel, SceneTargetWindow,
    WebLinkModel)
from karabo.native import Hash
from karabogui.events import KaraboEvent
from karabogui.testing import GuiTestCase, click_button, singletons

from ..link import (
    DeviceSceneLinkWidget, SceneLinkWidget, WebDialog, WebLinkWidget)


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
            self.widget.actions()[0].trigger()
            self.assertEqual(self.model.text, "This is a label text")

        path = "karabogui.sceneview.widget.link.SceneLinkDialog"
        with mock.patch(path) as dialog:
            dialog.exec.return_value = QDialog.Accepted
            dialog().selectedScene = "New selected scene"
            dialog().selectedTargetWindow = SceneTargetWindow.Dialog
            self.widget.actions()[1].trigger()
            self.assertEqual(self.model.target, "New selected scene")
            self.assertEqual(self.model.target_window,
                             SceneTargetWindow.Dialog)
            self.assertEqual(self.widget.toolTip(), "name:122345677")


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
            self.widget.actions()[0].trigger()
            self.assertEqual(self.model.text, "This is a label text")

        path = "karabogui.sceneview.widget.link.WebDialog"
        with mock.patch(path) as dialog:
            dialog.exec.return_value = QDialog.Accepted
            dialog().target = "www.desy.de"
            self.widget.actions()[1].trigger()
            self.assertEqual(self.model.target, "www.desy.de")
            self.assertEqual(self.widget.toolTip(), "www.xfel.eu")

    def test_webdialog(self):
        dialog = WebDialog(self.model.target)
        self.assertNotEqual(dialog.target, self.model.target)
        # Dialog corrects with http
        self.assertEqual(dialog.target, "http://www.xfel.eu")


def test_device_scene_link_widget(gui_app, mocker):
    model = DeviceSceneLinkModel(
        text="DeviceSceneLink Text", foreground="black",
        x=0, y=0, width=100, height=100,
        target="scene", keys=["XHQ_EH_DG/SCENE/LINK"])

    main_widget = QWidget()
    widget = DeviceSceneLinkWidget(model=model,
                                   parent=main_widget)
    main_widget.show()

    model_rect = QRect(model.x, model.y,
                       model.width, model.height)
    assert widget.rect() == model_rect
    assert len(widget.actions()) == 2

    # Geometry
    rect = QRect(2, 10, 2, 10)
    assert widget.geometry() != rect
    widget.set_geometry(rect)
    assert widget.geometry() == rect

    # Translation
    assert widget.pos() == QPoint(2, 10)
    widget.translate(QPoint(10, 0))
    assert widget.pos() == QPoint(12, 10)

    path = "karabogui.sceneview.widget.link.TextDialog"
    dialog = mocker.patch(path)
    dialog.exec.return_value = QDialog.Accepted
    dialog().label_model = LabelModel(text="This is a label text")
    widget.actions()[0].trigger()
    assert model.text == "This is a label text"

    path = "karabogui.sceneview.widget.link.DeviceCapabilityDialog"
    dialog = mocker.patch(path)
    dialog.exec.return_value = QDialog.Accepted
    dialog().capa_name = "plot"
    widget.actions()[1].trigger()
    assert model.target == "plot"
    assert widget.toolTip() == "XHQ_EH_DG/SCENE/LINK|scene"

    # Make sure, basic interface is there
    widget.add_proxies(None)
    widget.apply_changes()
    widget.decline_changes()
    widget.set_visible(True)
    widget.update_global_access_level(None)
    widget.destroy()


def test_device_scene_link_sending(gui_app, mocker):
    # Send a scene target
    model = DeviceSceneLinkModel(
        text="DeviceSceneLink Text", foreground="black",
        x=0, y=0, width=100, height=100,
        target="scene", keys=["XHQ_EH_DG/SCENE/LINK"])

    main_widget = QWidget()
    widget = DeviceSceneLinkWidget(model=model,
                                   parent=main_widget)
    main_widget.show()
    manager = mocker.Mock()
    path = "karabogui.sceneview.widget.link.is_device_online"
    question = mocker.patch(path)
    question.return_value = True
    with singletons(manager=manager):
        click_button(widget)
        args = manager.callDeviceSlot.call_args
        assert len(args) == 2
        assert args[0][1] == "XHQ_EH_DG/SCENE/LINK"
        assert args[0][2] == "requestScene"
        # of args the last argument is the parameter hash
        h = args[0][-1]
        assert isinstance(h, Hash)
        assert "name" in h
        assert h["name"] == "scene"

        # Erase target and click again, empty string is send
        model.target = ""
        click_button(widget)
        args = manager.callDeviceSlot.call_args
        h = args[0][-1]
        assert isinstance(h, Hash)
        assert "name" in h
        assert h["name"] == ""

    widget.destroy()


if __name__ == "__main__":
    main()
