# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main

from karabo.common.project.api import (
    DeviceInstanceModel, DeviceServerModel, MacroModel, ProjectModel)
from karabo.common.scenemodel.api import SceneModel
from karabogui.singletons.project_model import ProjectViewItemModel
from karabogui.testing import GuiTestCase


class TestProjectModel(GuiTestCase):
    def test_basic_project(self):
        """Test the QModelTest of the ProjectViewItemModel"""
        from pytestqt.modeltest import ModelTester

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

        model = ProjectViewItemModel()
        model.root_model = project_model
        tester = ModelTester(None)
        tester.check(model)


if __name__ == "__main__":
    main()
