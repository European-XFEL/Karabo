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
from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.project.api import MacroModel, ProjectModel
from karabogui.binding.api import ProxyStatus
from karabogui.singletons.mediator import Mediator
from karabogui.singletons.project_model import ProjectViewItemModel
from karabogui.testing import singletons

PATH_RUN_MACRO = "karabogui.project.controller.macro.run_macro"
MBOX_PATH = "karabogui.project.controller.macro.messagebox.show_warning"


def _get_subgroup_controllers(proj_controller, name):
    return [child for child in proj_controller.children
            if child.trait_name == name]


def _get_object_controllers(subgroup_controllers):
    controllers = []
    for subgroup in subgroup_controllers:
        controllers.extend(subgroup.children)
    return controllers


def test_macro_controller(gui_app, mocker):
    """Test the macro controller of the project"""
    push_exception_handler(lambda *args: None, reraise_exceptions=True)
    qt_model = ProjectViewItemModel(parent=None)

    # Instantiate a macro model
    simple_name = "macro_foo"
    uuid = "12345678-1234-5678-1234-567812345678"
    macro_model = MacroModel(simple_name=simple_name, uuid=uuid)
    macro_model.initialized = macro_model.modified = True

    def _assert_macro(code, is_valid):
        models = {"macros": [macro_model]}
        proj_model = ProjectModel(**models)

        # Cause the controllers to be created and get a ref to the root
        qt_model.root_model = proj_model
        proj_controller = qt_model.root_controller
        subgroup_controllers = _get_subgroup_controllers(proj_controller,
                                                         "macros")

        macro_controllers = _get_object_controllers(subgroup_controllers)
        macro_controller = macro_controllers[0]
        macro_model.code = code

        mocked_run_macro = mocker.patch(PATH_RUN_MACRO)
        mocked_messagebox = mocker.patch(MBOX_PATH)
        macro_controller.run_macro()
        if is_valid:
            mocked_run_macro.assert_called_once()
            mocked_messagebox.assert_not_called()
        else:
            mocked_run_macro.assert_not_called()
            mocked_messagebox.assert_called_once()

        class_id = simple_name.title()
        device_id = macro_model.instance_id
        devices = [(device_id, class_id, ProxyStatus.OFFLINE)]
        servers = []
        macro_controller.system_topology_callback(devices, servers)
        for child in macro_controller.children:
            assert child.ui_data.status is ProxyStatus.OFFLINE
            assert child.model is not macro_controller.model

    # Put here the assertion of the macro code
    with singletons(mediator=Mediator()):
        _assert_macro(VALID_MACRO, is_valid=True)
        _assert_macro(SYNTAX_ERROR_MACRO, is_valid=False)
        _assert_macro(IMPORT_ERROR_MACRO, is_valid=False)
        _assert_macro(INDENTATION_ERROR_MACRO, is_valid=False)

    pop_exception_handler()
    qt_model = None


VALID_MACRO = """
from karabo.middlelayer import Macro, Slot, String

class ValidMacro(Macro):
    name = String(defaultValue="Foo")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(name))

    @MacroSlot()
    def executeSlow(self):
        print("Hello {{}}!".format(name))
"""

SYNTAX_ERROR_MACRO = """
from karabo.middlelayer import Macro, Slot, String

class InvalidMacro(Macro)
    name = String(defaultValue="Foo")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(name))
"""

IMPORT_ERROR_MACRO = """
from karabo.middlelayer import Macro123, Slot, String

class InvalidMacro(Macro)
    name = String(defaultValue="Foo")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(name))
"""

INDENTATION_ERROR_MACRO = """
from karabo.middlelayer import Macro, Slot, String

  class InvalidMacro(Macro)
    name = String(defaultValue="Foo")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(name))
"""
