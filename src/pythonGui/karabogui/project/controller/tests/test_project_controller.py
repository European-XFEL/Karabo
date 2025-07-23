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
from karabo.common.project.api import ProjectModel
from karabo.common.project.device import DeviceInstanceModel
from karabo.common.project.device_config import DeviceConfigurationModel
from karabo.common.project.server import DeviceServerModel
from karabo.native import Hash
from karabogui.project.controller.device import DeviceInstanceController
from karabogui.singletons.mediator import Mediator
from karabogui.singletons.project_model import ProjectViewItemModel
from karabogui.testing import singletons

STATUS_ICON = 'karabogui.project.controller.' \
              'device.get_project_device_status_icon'


def test_active_configuration(gui_app):
    with singletons(mediator=Mediator()):
        # Changing configurations might emit signals
        fconfig = Hash()
        fconfig['fkey'] = 'fvalue'
        fconfig['skey'] = 'svalue'
        fconfig = DeviceConfigurationModel(class_id='BazClass',
                                           configuration=fconfig)
        fconfig.initialized = True
        sconfig = Hash()
        sconfig['fkey'] = 'value_diff'
        sconfig = DeviceConfigurationModel(class_id='BazClass',
                                           configuration=sconfig)
        sconfig.initialized = True
        config_list = [fconfig, sconfig]
        model = DeviceInstanceModel(class_id='BazClass',
                                    instance_id='fooDevice',
                                    configs=config_list)
        model.initialized = True
        model.active_config_ref = fconfig.uuid
        DeviceServerModel(server_id='testServer', host='serverFoo',
                          devices=[model])

        def assert_active_configuration():
            assert 'fkey' in fconfig.configuration
            assert fconfig.configuration['fkey'] == 'fvalue'
            assert 'skey' in fconfig.configuration
            assert fconfig.configuration['skey'] == 'svalue'
            assert 'fkey' in sconfig.configuration
            assert sconfig.configuration['fkey'] == 'value_diff'
            assert 'skey' not in sconfig.configuration

        controller = DeviceInstanceController(model=model)
        controller.active_config_changed(sconfig)
        assert_active_configuration()
        controller.active_config_changed(fconfig)
        assert_active_configuration()
        controller.active_config_changed(sconfig)
        assert_active_configuration()


def test_menu(gui_app, mocker):
    _tooltip = "Requires minimum 'OPERATOR' access level"
    proj = ProjectModel(scenes=[], servers=[],
                        subprojects=[])

    qt_model = ProjectViewItemModel(parent=None)
    # Cause the controllers to be created and get a ref to the root
    qt_model.root_model = proj
    controller = qt_model.root_controller

    # Macro group menu-items
    macro_group = controller.children[0]
    assert macro_group.group_name == "Macros"
    mock_project_enabled = mocker.patch(
        "karabogui.project.controller.project_groups.access_role_allowed")
    mock_project_enabled.return_value = False
    menu = macro_group.context_menu(macro_group)

    actions = menu.actions()
    assert not any([act.isEnabled() for act in actions])
    arrange_macros = actions[3]
    assert arrange_macros.text() == "Arrange Macros"
    assert arrange_macros.toolTip() == _tooltip

    mock_project_enabled = mocker.patch(
        "karabogui.project.controller.project_groups.access_role_allowed")
    mock_project_enabled.return_value = True
    menu = macro_group.context_menu(macro_group)
    actions = menu.actions()
    assert all([act.isEnabled() for act in actions])

    # Scene group menu-items
    scene_group = controller.children[1]
    assert scene_group.group_name == "Scenes"

    mock_project_enabled = mocker.patch(
        "karabogui.project.controller.project_groups.access_role_allowed")
    mock_project_enabled.return_value = False
    menu = scene_group.context_menu(scene_group)

    actions = menu.actions()
    arrange_scenes = actions[6]
    assert arrange_scenes.text() == "Arrange Scenes"
    assert arrange_scenes.toolTip() == _tooltip
    assert not arrange_scenes.isEnabled()
    about = actions[-1]
    assert about.text() == "About"
    assert about.isEnabled()

    load_scene = actions[1]
    assert load_scene.text() == "Load scene..."
    assert not load_scene.isEnabled()
    assert load_scene.toolTip() == "Requires minimum 'EXPERT' access level"

    mock_project_enabled = mocker.patch(
        "karabogui.project.controller.project_groups.access_role_allowed")
    mock_project_enabled.return_value = True
    menu = scene_group.context_menu(scene_group)
    actions = menu.actions()
    assert all([act.isEnabled() for act in actions])
