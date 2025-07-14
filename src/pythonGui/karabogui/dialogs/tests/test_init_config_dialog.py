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
from qtpy.QtCore import QModelIndex, Qt
from qtpy.QtWidgets import QDialog, QMessageBox

from karabo.native import AccessLevel, Timestamp
from karabogui.dialogs.api import (
    InitConfigurationDialog, SaveConfigurationDialog)
from karabogui.events import KaraboEvent
from karabogui.singletons.mediator import Mediator
from karabogui.testing import (
    access_level, click_button, singletons, system_hash)
from karabogui.topology.api import SystemTopology


def get_config_items():
    items = []
    for i in range(4):
        config = {}
        config["name"] = f"default{i}"
        config["timestamp"] = Timestamp().toLocal(" ", "seconds")
        items.append(config)
    return items


def test_init_config_dialog(gui_app, mocker):
    mediator = Mediator()
    network = mocker.Mock()
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(mediator=mediator, network=network, topology=topology):
        dialog = InitConfigurationDialog(instance_id="divvy")
        assert dialog.ui_instance.text() == "Device Id: divvy"
        # List of configurations is asked for!
        assert network.onListInitConfigurations.call_count == 1

        # Clicking the show device requests navigation panel to show
        path = "karabogui.dialogs.init_configuration.broadcast_event"
        broadcast = mocker.patch(path)
        click_button(dialog.ui_show_device)
        broadcast.assert_called_with(
            KaraboEvent.ShowDevice, {"deviceId": "divvy",
                                     "showTopology": True})

        # Let configurations arrive! First empty!
        event = {"items": [], "deviceId": "divvy"}
        dialog._event_list_updated(event)
        assert dialog.ui_status.text() == "No configurations are available!"

        # Now a set of configurations
        event = {"items": get_config_items(), "deviceId": "divvy"}
        dialog._event_list_updated(event)
        assert dialog.ui_status.text() == ""

        model = dialog.model
        source_model = model.sourceModel()
        assert model.rowCount(QModelIndex()) == 4
        assert source_model.rowCount(QModelIndex()) == 4

        # Change a filter setting
        dialog.ui_filter.setText("1")
        assert model.rowCount(QModelIndex()) == 1
        assert source_model.rowCount(QModelIndex()) == 4

        # Select a cell and request
        dialog.ui_table_widget.selectRow(0)
        # XXX: Unfortunately, QTest doubleClick is not working [Bug]
        dialog._request_configuration()
        network.onGetInitConfiguration.assert_called_with(
            "divvy", "default1", False)

        # Click refresh button
        click_button(dialog.ui_button_refresh)
        network.onListInitConfigurations.assert_called_with("divvy")

        # ContextMenu
        click_button(dialog.ui_table_widget, Qt.RightButton)

        # Delete action
        QMessageBox.exec = mocker.Mock(return_value=QMessageBox.No)
        dialog._request_delete()
        network.onDeleteInitConfiguration.call_count == 0

        QMessageBox.exec = mocker.Mock(return_value=QMessageBox.Yes)
        dialog._request_delete()
        network.onDeleteInitConfiguration.assert_called_with(
            "divvy", "default1")

        # Unregister and set successful!
        dialog.done(1)

        # Network event and close
        dialog._event_network({"status": False})

        index = source_model.index(0, 0)
        assert index.data() == "default0"

        save = "karabogui.dialogs.init_configuration." \
               "SaveConfigurationDialog"
        dia = mocker.patch(save)
        dia().name = "Next"
        dia().exec.return_value = QDialog.Accepted
        # Call it!
        dialog.open_save_dialog()
        network.onSaveInitConfiguration.assert_called_with(
            "Next", ["divvy"], update=True)


def test_config_save_dialog(gui_app, mocker):
    """Test the save config dialog"""
    mediator = Mediator()
    with singletons(mediator=mediator):
        dialog = SaveConfigurationDialog()

        # Network event and close
        mediator.unregister_listener = mocker.Mock()
        dialog.done(1)
        mediator.unregister_listener.assert_called_once()
        # Asserts simple defaults
        assert dialog.name == ""

        # Network event should be there and closes
        dialog._event_network({"status": False})


def test_delete_configuration(gui_app, mocker):
    network = mocker.Mock()
    with singletons(network=network), access_level(AccessLevel.EXPERT):
        dialog = InitConfigurationDialog(instance_id="divvy")
        assert not dialog.ui_button_delete.isEnabled()
        event = {"items": get_config_items(), "deviceId": "divvy"}
        dialog._event_list_updated(event)

        dialog.ui_table_widget.selectRow(0)
        assert dialog.ui_button_delete.isEnabled()
        QMessageBox.exec = mocker.Mock(return_value=QMessageBox.Yes)
        click_button(dialog.ui_button_delete)
        network.onDeleteInitConfiguration.assert_called_with(
            "divvy", "default0")
