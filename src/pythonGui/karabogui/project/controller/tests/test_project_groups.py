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
from unittest.mock import patch

from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.project.api import MacroModel, ProjectModel
from karabo.common.scenemodel.api import SceneModel
from karabogui.project.controller.project_groups import (
    _load_macro, _load_scene)
from karabogui.singletons.project_model import ProjectViewItemModel
from karabogui.testing import GuiTestCase


class TestProjectGroupController(GuiTestCase):

    def setUp(self):
        super().setUp()
        push_exception_handler(lambda *args: None, reraise_exceptions=True)
        self.qt_model = ProjectViewItemModel(parent=None)

        # Instantiate project controller
        proj_model = ProjectModel()
        self.qt_model.root_model = proj_model
        self.proj_controller = self.qt_model.root_controller

    def tearDown(self):
        super().tearDown()
        pop_exception_handler()
        self.qt_model = None

    def test_load_macro(self):
        self._assert_load_macro_name("hello", is_valid=True)
        self._assert_load_macro_name("foo@bar.baz", is_valid=False)

    def test_load_scene(self):
        self._assert_load_scene_name("hello", is_valid=True)
        self._assert_load_scene_name("foo@bar.baz", is_valid=False)

    def _assert_load_macro_name(self, name, is_valid):
        patched_get_config = patch(
            "karabogui.project.controller.project_groups.get_config",
            return_value={"data_dir": ""})

        patched_get_filename = patch(
            "karabogui.project.controller.project_groups.getOpenFileName",
            return_value=name + ".py")

        patched_read_model = patch(
            "karabogui.project.controller.project_groups.read_macro",
            return_value=MacroModel())

        patched_show_filename_error = patch(
            "karabogui.project.controller.project_groups.show_filename_error")

        with patched_get_config, patched_get_filename, \
                patched_read_model, patched_show_filename_error:
            _load_macro(self.proj_controller)

        macros = [macro.simple_name
                  for macro in self.proj_controller.model.macros]

        if is_valid:
            assert name in macros
        else:
            assert name not in macros

    def _assert_load_scene_name(self, name, is_valid):
        patched_get_config = patch(
            "karabogui.project.controller.project_groups.get_config",
            return_value={"data_dir": ""})

        patched_get_filename = patch(
            "karabogui.project.controller.project_groups.getOpenFileName",
            return_value=name + ".svg")

        patched_read_scene = patch(
            "karabogui.project.controller.project_groups.read_scene",
            return_value=SceneModel())

        patched_show_filename_error = patch(
            "karabogui.project.controller.project_groups.show_filename_error")

        with patched_get_config, patched_get_filename, \
                patched_read_scene, patched_show_filename_error:
            _load_scene(self.proj_controller)

        scenes = [scene.simple_name
                  for scene in self.proj_controller.model.scenes]

        if is_valid:
            assert name in scenes
        else:
            assert name not in scenes
