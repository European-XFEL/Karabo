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
from unittest import main

from karabo.native import Hash
from karabogui.binding.api import ProxyStatus
from karabogui.project.dialog.device_configuration import (
    DeviceConfigurationDialog)
from karabogui.testing import (
    GuiTestCase, get_device_schema, singletons, system_hash)
from karabogui.topology.api import SystemTopology


class TestProjectHandleDialog(GuiTestCase):
    def test_dialog_invalid_conf(self):
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(topology=topology):
            # Create the basic project device
            project_device = topology.get_project_device(
                device_id="divvy", class_id="FooClass", server_id="swerver")
            topology.ensure_proxy_class_schema(
                device_id="divvy", class_id="FooClass", server_id="swerver")
            assert project_device.status == ProxyStatus.ONLINE
            topology.class_schema_updated(server_id="swerver",
                                          class_id="FooClass",
                                          schema=get_device_schema())

            # 1. First dialog with config invalid
            with self.subTest("Test with invalid config key, erase path"):
                config = Hash("doubleProperty", [1.2, 2.4])
                dialog = DeviceConfigurationDialog(
                    name="default", configuration=config,
                    project_device=project_device)

                assert dialog.windowTitle() == \
                       "Configuration default - divvy"
                assert config == dialog.configuration

                # We have a single invalid path
                assert len(dialog._invalid_paths) == 1

                assert dialog.ui_num_paths.text() == "# 1"
                assert dialog.ui_paths.count() == 1
                assert dialog.ui_paths.currentText() == \
                       "doubleProperty"

                # Erase invalid path
                self.click(dialog.ui_erase_path)
                assert dialog.ui_num_paths.text() == "# 0"
                assert dialog.ui_paths.count() == 0
                assert dialog.ui_paths.currentText() == ""

            with self.subTest("Test with invalid config key, sanitize"):
                config = Hash("doubleProperty", [1.2, 2.4])
                dialog = DeviceConfigurationDialog(
                    name="default", configuration=config,
                    project_device=project_device)
                assert dialog.ui_num_paths.text() == "# 1"
                assert dialog.ui_paths.count() == 1
                assert dialog.ui_paths.currentText() == \
                       "doubleProperty"

                assert len(dialog._invalid_paths) == 1

                # Sanitize
                self.click(dialog.ui_sanitize)
                assert dialog.ui_num_paths.text() == "# 0"
                assert dialog.ui_paths.count() == 0
                assert dialog.ui_paths.currentText() == ""

            with self.subTest("Erase all, even valid keys"):
                config = Hash("notinschema", False, "doubleProperty",
                              [False, 2.1], "stringProperty", "bar")
                dialog = DeviceConfigurationDialog(
                    name="default", configuration=config,
                    project_device=project_device)
                assert dialog.ui_num_paths.text() == "# 3"
                assert dialog.ui_paths.count() == 3
                assert dialog.ui_paths.currentText() == \
                       "notinschema"

                # A single invalid one
                assert len(dialog._invalid_paths) == 1

                # Erase all
                self.click(dialog.ui_erase_all)
                assert dialog.ui_num_paths.text() == "# 0"
                assert dialog.ui_paths.count() == 0
                assert dialog.ui_paths.currentText() == ""

                # Result is empty configuration after erase
                assert dialog.configuration.empty()

            with self.subTest(msg="Test with valid empty config"):
                config = Hash()
                dialog = DeviceConfigurationDialog(
                    name="default", configuration=config,
                    project_device=project_device)

                # Everything is fine
                assert dialog.ui_num_paths.text() == "# 0"
                assert dialog.ui_paths.count() == 0
                assert dialog.ui_paths.currentText() == ""

                assert dialog.ui_valid_info.toPlainText() == \
                       "No conflicts in configuration!"

            with self.subTest(msg="Test with attribute removal on and off"):
                config = Hash("doubleProperty", 1.2)
                config.setAttribute("doubleProperty", "alarmLow", 5.2)
                dialog = DeviceConfigurationDialog(
                    name="default", configuration=config,
                    project_device=project_device)
                assert dialog.ui_remove_attributes.isChecked()

                # Everything is fine
                assert dialog.ui_num_paths.text() == "# 1"
                assert dialog.ui_paths.count() == 1
                assert dialog.ui_paths.currentText() == \
                       "doubleProperty"
                # No conflicts, but sanitize will remove attributes
                # and still a reconfigurable is left over
                self.click(dialog.ui_sanitize)
                assert dialog.ui_num_paths.text() == "# 1"
                assert dialog.ui_paths.count() == 1
                assert dialog.ui_paths.currentText() == \
                       "doubleProperty"
                conf = dialog.configuration
                attributes = conf["doubleProperty", ...]
                assert "alarmLow" not in attributes

                # Now test without removing attributes
                config = Hash("doubleProperty", 1.2)
                config.setAttribute("doubleProperty", "alarmLow", 5.2)
                dialog = DeviceConfigurationDialog(
                    name="default", configuration=config,
                    project_device=project_device)
                dialog.ui_remove_attributes.setChecked(False)
                assert not dialog.ui_remove_attributes.isChecked()
                self.click(dialog.ui_sanitize)
                assert dialog.ui_paths.count() == 1


if __name__ == "__main__":
    main()
