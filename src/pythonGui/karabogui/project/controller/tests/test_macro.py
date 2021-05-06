from unittest.mock import patch

from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.project.api import MacroModel, ProjectModel
from karabogui.singletons.project_model import ProjectViewItemModel
from karabogui.testing import GuiTestCase


class MacroControllerTestCase(GuiTestCase):

    def setUp(self):
        super(MacroControllerTestCase, self).setUp()
        push_exception_handler(lambda *args: None, reraise_exceptions=True)
        self.qt_model = ProjectViewItemModel(parent=None)

        # Instantiate a macro model
        self.simple_name = "macro_foo"
        self.uuid = "12345678-1234-5678-1234-567812345678"
        self.macro_model = MacroModel(simple_name=self.simple_name,
                                      uuid=self.uuid)
        self.device_id = self.macro_model.instance_id
        self.macro_model.initialized = self.macro_model.modified = True

    def tearDown(self):
        super(MacroControllerTestCase, self).tearDown()
        pop_exception_handler()
        self.qt_model = None

    def test_macro_validity(self):
        self._assert_macro(VALID_MACRO, is_valid=True)
        self._assert_macro(SYNTAX_ERROR_MACRO, is_valid=False)
        self._assert_macro(IMPORT_ERROR_MACRO, is_valid=False)
        self._assert_macro(INDENTATION_ERROR_MACRO, is_valid=False)

    def _assert_macro(self, code, is_valid):
        macro_controllers = self._create_controllers("macros",
                                                     [self.macro_model])
        macro_controller = macro_controllers[0]

        run_macro_path = "karabogui.project.controller.macro.run_macro"
        messagebox_path = "karabogui.project.controller.macro.messagebox" \
                          ".show_warning"

        self.macro_model.code = code
        with patch(run_macro_path) as mocked_run_macro, \
                patch(messagebox_path) as mocked_messagebox:
            macro_controller.run_macro()
            if is_valid:
                mocked_run_macro.assert_called_once()
                mocked_messagebox.assert_not_called()
            else:
                mocked_run_macro.assert_not_called()
                mocked_messagebox.assert_called_once()

    def _create_controllers(self, type_, models):
        models = {type_: models}
        proj_controller = self._create_project_controller(models)
        subgroup_controllers = self._get_subgroup_controllers(proj_controller,
                                                              type_)
        return self._get_object_controllers(subgroup_controllers)

    def _create_project_controller(self, models):
        # Instantiate project controller
        proj_model = ProjectModel(**models)

        # Cause the controllers to be created and get a ref to the root
        self.qt_model.root_model = proj_model
        return self.qt_model.root_controller

    def _get_subgroup_controllers(self, proj_controller, name):
        return [child for child in proj_controller.children
                if child.trait_name == name]

    def _get_object_controllers(self, subgroup_controllers):
        controllers = []
        for subgroup in subgroup_controllers:
            controllers.extend(subgroup.children)
        return controllers

    def _mock_macro_status(self, macro_controller, simple_name, device_id,
                           status):
        # Mock that the device is now online
        class_id = simple_name.title()
        devices = [(device_id, class_id, status)]
        servers = []
        macro_controller.system_topology_callback(devices, servers)


VALID_MACRO = """
from karabo.middlelayer import Macro, Slot, String

class ValidMacro(Macro):
    name = String(defaultValue="Foo")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(self.name))
"""

SYNTAX_ERROR_MACRO = """
from karabo.middlelayer import Macro, Slot, String

class InvalidMacro(Macro)
    name = String(defaultValue="Foo")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(self.name))
"""


IMPORT_ERROR_MACRO = """
from karabo.middlelayer import Macro123, Slot, String

class InvalidMacro(Macro)
    name = String(defaultValue="Foo")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(self.name))
"""

INDENTATION_ERROR_MACRO = """
from karabo.middlelayer import Macro, Slot, String

  class InvalidMacro(Macro)
    name = String(defaultValue="Foo")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(self.name))
"""
