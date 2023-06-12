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
from collections import namedtuple

from qtpy import uic
from qtpy.QtCore import (
    QAbstractTableModel, QItemSelection, QSortFilterProxyModel, Qt, Slot)
from qtpy.QtGui import QPalette
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QHeaderView

import karabogui.icons as icons
from karabogui import messagebox
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.singletons.api import get_network, get_topology
from karabogui.validators import RegexValidator

from .utils import get_dialog_ui

NAME_FIELD = 0
PRIORITY_FIELD = 2
DESCRIPTION_FIELD = 4

PRIORITY_TEXT = ["TEST", "COMMISSIONED", "SAFE"]
PRIORITY_EXP = [
    "The configuration is either transient or only used for testing.",
    "The configuration is declared as commissioned and defines a possible "
    "setting this device can operate with.",
    "A safe configuration can be used for start up of this device."
]


class SaveConfigurationDialog(QDialog):
    """Save dialog for configuration details"""

    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("config_save.ui"), self)
        self.ui_priority.currentIndexChanged.connect(self._change_text)
        index = self.ui_priority.currentIndex()
        self.ui_priority_text.setText(PRIORITY_EXP[index])
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.ui_name.textChanged.connect(self._check_button)
        # We allow only characters and numbers up to a length of 30!
        validator = RegexValidator(pattern="^[A-Za-z0-9_-]{1,30}$")
        self.ui_name.setValidator(validator)

        self._normal_palette = self.ui_name.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)

        self.event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(self.event_map)

    @Slot()
    def _check_button(self):
        """Basic validation of configuration name"""
        enable = self.ui_name.hasAcceptableInput()
        if enable:
            self.ui_name.setPalette(self._normal_palette)
        else:
            self.ui_name.setPalette(self._error_palette)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)

    @Slot(int)
    def _change_text(self, index):
        self.ui_priority_text.setText(PRIORITY_EXP[index])

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    def done(self, result):
        """Stop listening for broadcast events"""
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    @property
    def description(self):
        return self.ui_description.toPlainText()

    @property
    def priority(self):
        return int(self.ui_priority.currentIndex()) + 1

    @property
    def name(self):
        return self.ui_name.text()


class ConfigurationFromNameDialog(QDialog):
    """List configurations by ``name`` from the configuration database
    """

    def __init__(self, instance_id, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("config_handle.ui"), self)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        self.setWindowFlags(self.windowFlags() | Qt.WindowCloseButtonHint)

        self.instance_id = instance_id
        self.ui_instance.setText(f"Device Id: {instance_id}")

        self.event_map = {
            KaraboEvent.ListConfigurationUpdated: self._event_list_updated,
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(self.event_map)

        self.model = QSortFilterProxyModel(parent=self)
        self.model.setSourceModel(TableModel(parent=self))
        self.model.setFilterRole(Qt.DisplayRole)
        self.model.setFilterFixedString("")
        self.model.setFilterCaseSensitivity(False)
        self.model.setFilterKeyColumn(0)

        self.buttonBox.button(QDialogButtonBox.Ok).setText('Load')

        self.ui_table_widget.setModel(self.model)
        header = self.ui_table_widget.horizontalHeader()
        header.setSectionResizeMode(QHeaderView.ResizeToContents)
        header.setStretchLastSection(True)
        self.ui_table_widget.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.ui_table_widget.doubleClicked.connect(self._request_configuration)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.ui_filter.textChanged.connect(self._filter_changed)
        self.ui_button_clear.clicked.connect(self.ui_filter.clear)

        self.ui_filter.setFocus()

        self.ui_button_save.setIcon(icons.save)
        self.ui_show_device.setIcon(icons.deviceInstance)
        self.ui_button_refresh.setIcon(icons.reset)
        # Provide saving option
        self.ui_button_save.clicked.connect(self.open_save_dialog)
        # Show device option!
        self.ui_show_device.clicked.connect(self._show_device)

        # Request fresh at startup!
        get_network().onListConfigurationFromName(device_id=self.instance_id)

    def _event_list_updated(self, data):
        instance_id = data["deviceId"]
        if instance_id != self.instance_id:
            return

        model = self.model.sourceModel()
        items = data["items"]
        if not items:
            self.ui_status.setText("No configurations are available!")
        else:
            self.ui_status.setText("")
        model.set_internal_data(items)

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    def done(self, result):
        """Stop listening for broadcast events"""
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    def _check_existing(self):
        if self.ui_table_widget.selectionModel().hasSelection():
            return True

        return False

    def _check_button_state(self):
        enable = self._check_existing()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)

    # --------------------------------------------------------------------
    # Qt Slots

    @Slot()
    def _show_device(self):
        broadcast_event(KaraboEvent.ShowDevice, {'deviceId': self.instance_id,
                                                 'showTopology': True})

    @Slot()
    def open_save_dialog(self):
        dialog = SaveConfigurationDialog(parent=self)
        if dialog.exec() == QDialog.Accepted:
            device = get_topology().get_device(self.instance_id)
            if not device.online:
                text = (f"The device {self.instance_id} is not online, the "
                        f"configuration cannot be saved.")
                messagebox.show_alarm(text, parent=self)
                return
            priority = dialog.priority
            description = dialog.description
            name = dialog.name
            get_network().onSaveConfigurationFromName(
                name, [self.instance_id], priority=priority,
                description=description, update=True)

    @Slot()
    def accept(self):
        """The dialog was accepted and we can request a configuration"""
        self._request_configuration()
        super().accept()

    @Slot()
    def _request_configuration(self):
        """Request the configuration for the actual `instance_id`"""
        index = self.ui_table_widget.selectionModel().selectedRows()[0]
        if not index.isValid():
            return
        model = index.model()
        name = model.index(index.row(), NAME_FIELD).data()
        preview = self.ui_preview.isChecked()
        get_network().onGetConfigurationFromName(self.instance_id, name,
                                                 preview)

    @Slot()
    def on_ui_button_refresh_clicked(self):
        """Refresh the list of configurations for the actual `instance_id`"""
        get_network().onListConfigurationFromName(self.instance_id)

    @Slot(str)
    def _filter_changed(self, text):
        self.model.setFilterFixedString(text)
        self._check_button_state()

    @Slot(QItemSelection, QItemSelection)
    def _selectionChanged(self, selected, deselected):
        self._check_button_state()


# Keys to mangle capital letters
CONFIG_DB_NAME = "name"
CONFIG_DB_TIMEPOINT = "timepoint"
CONFIG_DB_PRIORITY = "priority"
CONFIG_DB_USER = "user"
CONFIG_DB_DESCRIPTION = "description"

CONFIG_DATA = {}
CONFIG_DATA[CONFIG_DB_NAME] = "Name"
CONFIG_DATA[CONFIG_DB_TIMEPOINT] = "Timepoint"
CONFIG_DATA[CONFIG_DB_PRIORITY] = "Priority"
CONFIG_DATA[CONFIG_DB_USER] = "User"
CONFIG_DATA[CONFIG_DB_DESCRIPTION] = "Description"

ConfigurationEntry = namedtuple("ConfigurationEntry", list(CONFIG_DATA.keys()))


class TableModel(QAbstractTableModel):
    headers = [CONFIG_DATA[CONFIG_DB_NAME],
               CONFIG_DATA[CONFIG_DB_TIMEPOINT],
               CONFIG_DATA[CONFIG_DB_PRIORITY]]

    def __init__(self, instance_id="", parent=None):
        super().__init__(parent)
        self.instance_id = instance_id
        self.data = []

    def set_internal_data(self, data):
        """ Add the given `data` to the internal data structure
        """
        self.beginResetModel()
        try:
            self.data = [
                ConfigurationEntry(
                    name=str(item["name"]),
                    timepoint=item["timepoint"],
                    priority=str(item["priority"]),
                    user=item["user"],
                    description=item["description"],
                ) for item in data]
        finally:
            self.endResetModel()

    def rowCount(self, parent=None):
        return len(self.data)

    def columnCount(self, parent=None):
        return len(self.headers)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None

        entry = self.data[index.row()]
        column = index.column()
        if role == Qt.DisplayRole:
            if column == PRIORITY_FIELD:
                return PRIORITY_TEXT[int(entry[column]) - 1]
            else:
                return entry[column]
        elif role == Qt.ToolTipRole:
            return entry[DESCRIPTION_FIELD]
        elif role == Qt.UserRole:
            return entry[column]
        return None

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]
