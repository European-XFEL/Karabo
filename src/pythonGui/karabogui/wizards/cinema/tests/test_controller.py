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
from unittest import mock

from qtpy.QtCore import Qt
from qtpy.QtGui import QKeyEvent
from qtpy.QtWidgets import QApplication

from karabo.common.project.api import ProjectModel
from karabo.common.scenemodel.api import SceneModel
from karabogui.testing import GuiTestCase
from karabogui.util import SignalBlocker

from ..controllers import ConfigureController, SelectScenesController
from ..wizard import CinemaWizardController


class NetworkMock:

    def __init__(self, host, port):
        self.hostname = host
        self.port = port

    def connectToServer(self):
        """Do nothing!"""


MOCKED_NETWORK = NetworkMock(host="not-localhost", port="12345")
GET_NETWORK_PATH = "karabogui.wizards.cinema.wizard.get_network"


class BaseCinemaInstallerTest(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.installer = CinemaWizardController()

    def tearDown(self):
        super().tearDown()
        self.installer = None

    def assert_select_scenes_page(self, available, selected):
        current_page = self.installer.current_controller.page

        # Assert available models
        items = self.get_listwidget_items(current_page.available_listwidget)
        avail_models = [item.data(Qt.UserRole) for item in items]
        assert avail_models == available

        current_controller = self.installer.current_controller
        assert current_controller.available_scenes == available

        # Assert selected models
        items = self.get_listwidget_items(current_page.selected_listwidget)
        sel_models = [item.data(Qt.UserRole) for item in items]
        assert sel_models == selected

        assert current_controller.selected_scenes == selected
        # Assert that the wizard can go next if there are selected items
        assert current_page.isComplete() == bool(selected)

    def assert_configure_link_page(self, **kwargs):
        # Defaults here that are updated
        config = dict(host=MOCKED_NETWORK.hostname,
                      port=MOCKED_NETWORK.port,
                      show_splash=True,
                      include_host=False)
        config.update(kwargs)

        current_controller = self.installer.current_controller
        # Assert shortcut config
        for k, v in config.items():
            assert current_controller.link[k] == v

        # Assert widget values
        current_page = current_controller.page
        assert current_page.splash_checkbox.isChecked() == \
               config["show_splash"]
        assert current_page.include_host_checkbox.isChecked() == \
               config["include_host"]
        assert current_page.host_lineedit.text() == config["host"]
        assert current_page.port_lineedit.text() == config["port"]

    def start_installer(self, **traits):
        self.installer.trait_set(**traits)
        self.installer.run()

    def get_listwidget_items(self, list_widget):
        return [list_widget.item(row)
                for row in range(list_widget.count())]


@mock.patch(GET_NETWORK_PATH, return_value=MOCKED_NETWORK)
class TestSelectScenes(BaseCinemaInstallerTest):

    def test_init(self, mocked_network):
        """Test the initialization of the Select Scenes Controller"""
        available_scenes = [SceneModel(simple_name="available_1"),
                            SceneModel(simple_name="available_2")]
        selected_scenes = [SceneModel(simple_name="selected_1"),
                           SceneModel(simple_name="selected_2")]
        project_model = ProjectModel(scenes=available_scenes + selected_scenes)

        # Start the installer with a project model and a selected scene.
        self.start_installer(project_model=project_model)
        # The wizard goes directly to the select scenes page
        current_controller = self.installer.current_controller
        assert isinstance(current_controller, SelectScenesController)

    def test_changes(self, mocked_network):
        """Test the changes (add/remove) of the Select Scenes Controller"""
        available_scenes = [SceneModel(simple_name="available_1"),
                            SceneModel(simple_name="available_2")]
        selected_scenes = [SceneModel(simple_name="selected_1"),
                           SceneModel(simple_name="selected_2")]
        project_model = ProjectModel(scenes=available_scenes + selected_scenes)

        # Start the installer with a project model and a selected scene.
        self.start_installer(project_model=project_model)
        self.assert_select_scenes_page(available_scenes + selected_scenes,
                                       selected=[])

        # Add scenes
        self._add_model(selected_scenes[0])
        self.assert_select_scenes_page(available_scenes + [selected_scenes[1]],
                                       selected=[selected_scenes[0]])
        self._add_model(selected_scenes[1])
        self.assert_select_scenes_page(available_scenes, selected_scenes)

        # Remove scenes
        self._remove_model(selected_scenes[0])
        self.assert_select_scenes_page(available_scenes + [selected_scenes[0]],
                                       selected=[selected_scenes[1]])
        self._remove_model(selected_scenes[1])
        self.assert_select_scenes_page(available_scenes + selected_scenes,
                                       selected=[])

    def test_next(self, mocked_network):
        """Test the `next` of the Select Scenes Controller"""
        available_scenes = [SceneModel(simple_name="available_1"),
                            SceneModel(simple_name="available_2")]
        selected_scenes = [SceneModel(simple_name="selected_1"),
                           SceneModel(simple_name="selected_2")]
        project_model = ProjectModel(scenes=available_scenes + selected_scenes)

        # Start the installer with a project model and select the items.
        self.start_installer(project_model=project_model)
        for scene in selected_scenes:
            self._add_model(scene)

        # Go next!
        self.installer.widget.next()
        current_controller = self.installer.current_controller
        assert isinstance(current_controller, ConfigureController)

        # Go back and check that nothing has changed
        self.installer.widget.back()
        current_controller = self.installer.current_controller
        assert isinstance(current_controller, SelectScenesController)
        self.assert_select_scenes_page(available_scenes, selected_scenes)

    def _add_model(self, model):
        widget = self.installer.current_controller.page
        self._select_item(model, widget.available_listwidget)
        widget.add_button.click()

    def _remove_model(self, model):
        widget = self.installer.current_controller.page
        self._select_item(model, widget.selected_listwidget)
        widget.remove_button.click()

    def _select_item(self, model, list_widget):
        for item in self.get_listwidget_items(list_widget):
            if item.data(Qt.UserRole) is model:
                return list_widget.setCurrentItem(item)


@mock.patch(GET_NETWORK_PATH, return_value=MOCKED_NETWORK)
class TestConfigureLink(BaseCinemaInstallerTest):

    def test_init_single_selected(self, mocked_network):
        """Test the initialization of ConfigureLinkController"""
        selected_scene = SceneModel(simple_name="selected")
        available_scene = SceneModel(simple_name="available")
        project_model = ProjectModel(scenes=[available_scene, selected_scene])

        # Start the installer with a project model and a selected scene.
        self.start_installer(selected_scenes=[selected_scene],
                             project_model=project_model)
        # The wizard goes directly to the configure shortcut page

        current_controller = self.installer.current_controller
        assert isinstance(current_controller, ConfigureController)
        self.assert_configure_link_page(uuids=[selected_scene.uuid])
        current_page = current_controller.page
        assert current_page.isComplete()

    def test_init_multiple_selected(self, mocked_network):
        """Test the setup of ConfigureLinkController with multiple scenes"""

        available_scenes = [SceneModel(simple_name="available_1"),
                            SceneModel(simple_name="available_2")]
        selected_scenes = [SceneModel(simple_name="selected_1"),
                           SceneModel(simple_name="selected_2")]
        project_model = ProjectModel(simple_name="Project",
                                     scenes=available_scenes + selected_scenes)

        # Start the installer with a project model and a selected scene.
        self.start_installer(selected_scenes=selected_scenes,
                             project_model=project_model)

        # The wizard goes directly to the configure shortcut page
        current_controller = self.installer.current_controller
        assert isinstance(current_controller, ConfigureController)
        self.assert_configure_link_page(
            uuids=[scene.uuid for scene in selected_scenes])
        current_page = current_controller.page
        assert current_page.isComplete()

    def test_changes(self, mocked_network):
        """Test the changes of the link configuration and validation"""
        selected_scene = SceneModel(simple_name="selected")
        available_scene = SceneModel(simple_name="available")
        project_model = ProjectModel(scenes=[available_scene, selected_scene])

        # Start the installer with a project model and a selected scene.
        self.start_installer(selected_scenes=[selected_scene],
                             project_model=project_model)

        page = self.installer.current_controller.page
        link = self.installer.current_controller.link

        # Toggle splash checkbox
        page.splash_checkbox.toggle()
        assert link["show_splash"] == page.splash_checkbox.isChecked()

        # Toggle login checkbox
        page.include_host_checkbox.toggle()
        is_checked = page.include_host_checkbox.isChecked()
        assert link["include_host"] == is_checked
        assert page.host_lineedit.isVisible() == is_checked
        assert page.port_lineedit.isVisible() == is_checked

        # Edit host field: invalid
        self._edit_lineedit('', page.host_lineedit)
        assert not page.isComplete()

        # Edit host field: valid
        self._edit_lineedit("some-host", page.host_lineedit)
        assert page.isComplete()
        assert link["host"] == "some-host"

        # Edit port field: invalid
        self._edit_lineedit('', page.port_lineedit)
        assert not page.isComplete()

        # Edit port: valid
        self._edit_lineedit("34567", page.port_lineedit)
        assert page.isComplete()
        assert link["port"] == "34567"

    def _edit_lineedit(self, text, line_edit):
        line_edit.setFocus()
        line_edit.setText(text)
        key_event = QKeyEvent(QKeyEvent.KeyPress, Qt.Key_Enter, Qt.NoModifier)
        with SignalBlocker(self.installer.widget):
            QApplication.sendEvent(line_edit, key_event)
