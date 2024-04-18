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

from pytestqt.modeltest import ModelTester

from karabo.common.project.api import (
    DeviceInstanceModel, DeviceServerModel, MacroModel, ProjectModel)
from karabo.common.scenemodel.api import SceneModel
from karabogui.project.view import ProjectView
from karabogui.singletons.project_model import ProjectViewItemModel
from karabogui.singletons.selection_tracker import SelectionTracker
from karabogui.testing import singletons


def test_basic_project(gui_app):
    """Test the QModelTest of the ProjectViewItemModel"""

    # 1. Macros
    macros = [MacroModel(simple_name="MyMacro", code="")]
    # 2. Scenes
    scenes = [SceneModel(simple_name="Empty Scene")]
    # 3. Devices
    device_karabo = DeviceInstanceModel(
        instance_id='Karabo')
    device_xfel = DeviceInstanceModel(
        instance_id='XFEL')
    # 4. Servers
    servers = [DeviceServerModel(devices=[device_karabo, device_xfel])]

    # 5. Nested subprojects
    subsubproject = ProjectModel(macros=macros)
    subprojects = [ProjectModel(macros=macros,
                                subprojects=[subsubproject])]
    # 6. Root model
    project_model = ProjectModel(
        macros=macros, scenes=scenes, servers=servers,
        subprojects=subprojects)
    tracker = SelectionTracker()
    with singletons(selection_tracker=tracker):
        model = ProjectViewItemModel()
        model.root_model = project_model
        with singletons(project_model=model):
            view = ProjectView()

        assert view.model() is model
        tester = ModelTester(None)
        tester.check(model)

        assert model.selectModel(device_karabo)
