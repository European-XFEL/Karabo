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

from qtpy.QtCore import QModelIndex, Qt
from qtpy.QtWidgets import QDialog

from karabo.native import AccessLevel, Timestamp
from karabogui.dialogs.api import (
    ConfigurationFromNameDialog, SaveConfigurationDialog)
from karabogui.events import KaraboEvent
from karabogui.singletons.mediator import Mediator
from karabogui.testing import (
    GuiTestCase, access_level, click_button, singletons, system_hash)
from karabogui.topology.api import SystemTopology


def get_config_items():
    items = []
    for i in range(4):
        config = {}
        config["name"] = f"default{i}"
        config["timepoint"] = Timestamp().toLocal()
        items.append(config)
    return items


class TestConfigurationFromNameDialog(GuiTestCase):

    def test_config_name_dialog(self):
        mediator = Mediator()
        network = mock.Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(mediator=mediator, network=network, topology=topology):
            dialog = ConfigurationFromNameDialog(instance_id="divvy")
            self.assertEqual(dialog.ui_instance.text(), "Device Id: divvy")
            # List of configurations is asked for!
            network.onListConfigurationFromName.assert_called_once()

            # Clicking the show device requests navigation panel to show
            path = "karabogui.dialogs.configuration_from_name.broadcast_event"
            with mock.patch(path) as broadcast:
                self.click(dialog.ui_show_device)
                broadcast.assert_called_with(
                    KaraboEvent.ShowDevice, {"deviceId": "divvy",
                                             "showTopology": True})

            # Let configurations arrive! First empty!
            event = {"items": [], "deviceId": "divvy"}
            dialog._event_list_updated(event)
            self.assertEqual(dialog.ui_status.text(),
                             "No configurations are available!")

            # Now a set of configurations
            event = {"items": get_config_items(), "deviceId": "divvy"}
            dialog._event_list_updated(event)
            self.assertEqual(dialog.ui_status.text(), "")

            model = dialog.model
            source_model = model.sourceModel()
            self.assertEqual(model.rowCount(QModelIndex()), 4)
            self.assertEqual(source_model.rowCount(QModelIndex()), 4)

            # Change a filter setting
            dialog.ui_filter.setText("1")
            self.assertEqual(model.rowCount(QModelIndex()), 1)
            self.assertEqual(source_model.rowCount(QModelIndex()), 4)

            # Select a cell and request
            dialog.ui_table_widget.selectRow(0)
            # XXX: Unfortunately, QTest doubleClick is not working [Bug]
            dialog._request_configuration()
            network.onGetConfigurationFromName.assert_called_with(
                "divvy", "default1", False)

            # Click refresh button
            self.click(dialog.ui_button_refresh)
            network.onListConfigurationFromName.assert_called_with("divvy")

            # ContextMenu
            click_button(dialog.ui_table_widget, Qt.RightButton)

            # Delete action
            dialog._request_delete()
            network.onDeleteConfigurationFromName.assert_called_with(
                "divvy", "default1")

            # Unregister and set successful!
            dialog.done(1)

            # Network event and close
            dialog._event_network({"status": False})

            index = source_model.index(0, 0)
            self.assertEqual(index.data(), "default0")

            save = "karabogui.dialogs.configuration_from_name." \
                   "SaveConfigurationDialog"
            with mock.patch(save) as dia:
                dia().name = "Next"
                dia().exec.return_value = QDialog.Accepted
                # Call it!
                dialog.open_save_dialog()
                network.onSaveConfigurationFromName.assert_called_with(
                    "Next", ["divvy"], update=True)

    def test_config_save_dialog(self):
        """Test the save config dialog"""
        mediator = Mediator()
        with singletons(mediator=mediator):
            dialog = SaveConfigurationDialog()

            # Network event and close
            mediator.unregister_listener = mock.Mock()
            dialog.done(1)
            mediator.unregister_listener.assert_called_once()
            # Asserts simple defaults
            self.assertEqual(dialog.name, "")

            # Network event should be there and closes
            dialog._event_network({"status": False})

    def test_delete_configuration(self):
        network = mock.Mock()
        with singletons(network=network), access_level(AccessLevel.EXPERT):
            dialog = ConfigurationFromNameDialog(instance_id="divvy")
            self.assertEqual(dialog.ui_button_delete.isEnabled(), False)
            event = {"items": get_config_items(), "deviceId": "divvy"}
            dialog._event_list_updated(event)

            dialog.ui_table_widget.selectRow(0)
            self.assertEqual(dialog.ui_button_delete.isEnabled(), True)
            click_button(dialog.ui_button_delete)
            network.onDeleteConfigurationFromName.assert_called_with(
                "divvy", "default0")


if __name__ == "__main__":
    main()
