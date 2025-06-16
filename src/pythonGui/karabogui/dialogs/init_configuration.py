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
    QAbstractTableModel, QItemSelection, QPoint, QSortFilterProxyModel, Qt,
    Slot)
from qtpy.QtGui import QPalette
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QHeaderView, QMenu

import karabogui.icons as icons
from karabogui import messagebox
from karabogui.access import (
    AccessRole, access_role_allowed, get_access_level_for_role)
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.singletons.api import get_network, get_topology
from karabogui.util import utc_to_local
from karabogui.validators import RegexValidator

from .utils import get_dialog_ui

NAME_COLUMN = 0


class SaveConfigurationDialog(QDialog):
    """Save dialog for configuration details"""

    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("init_config_save.ui"), self)
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

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    def done(self, result):
        """Stop listening for broadcast events"""
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    @property
    def name(self):
        return self.ui_name.text()


class InitConfigurationDialog(QDialog):
    """List configurations by ``name`` from the configuration database
    """

    def __init__(self, instance_id, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("init_configuration_dialog.ui"), self)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        self.setWindowFlags(self.windowFlags() | Qt.WindowCloseButtonHint)

        self.instance_id = instance_id
        self.ui_instance.setText(f"Device Id: {instance_id}")

        self.event_map = {
            KaraboEvent.ListConfigurationUpdated: self._event_list_updated,
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.AccessLevelChanged: self._event_access_level,
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
        self.ui_table_widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.ui_table_widget.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.ui_table_widget.doubleClicked.connect(self._request_configuration)
        self.ui_table_widget.customContextMenuRequested.connect(
            self.onCustomContextMenuRequested)

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

        self.ui_button_delete.setIcon(icons.delete)
        self.ui_button_delete.clicked.connect(self._request_delete)

        # Request fresh at startup!
        get_network().onListInitConfigurations(device_id=self.instance_id)

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
        self._check_button_state()

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    def _event_access_level(self, data):
        """Update the enabled status of Delete button on access level
        changes"""
        allow_deletion = access_role_allowed(AccessRole.CONFIGURATION_DELETE)
        enable = allow_deletion and self._check_existing()
        self.ui_button_delete.setEnabled(enable)

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
        enable = enable and access_role_allowed(
            AccessRole.CONFIGURATION_DELETE)
        self.ui_button_delete.setEnabled(enable)

    # --------------------------------------------------------------------
    # Qt Slots

    @Slot(QPoint)
    def onCustomContextMenuRequested(self, pos):
        rows = self.ui_table_widget.selectionModel().selectedRows()
        if not rows:
            return

        menu = QMenu(parent=self.ui_table_widget)
        delete_action = menu.addAction(icons.delete, "Delete")
        delete_action.triggered.connect(self._request_delete)
        allowed = access_role_allowed(AccessRole.CONFIGURATION_DELETE)
        if not allowed:
            delete_action.setEnabled(False)
            level = get_access_level_for_role(AccessRole.CONFIGURATION_DELETE)
            delete_action.setToolTip(
                f"Need access level {level} to delete configuration.")
        menu.exec(self.ui_table_widget.viewport().mapToGlobal(pos))

    @Slot()
    def _request_delete(self):
        index = self.ui_table_widget.selectionModel().selectedRows()[0]
        if not index.isValid():
            return
        model = index.model()
        name = model.index(index.row(), NAME_COLUMN).data()
        get_network().onDeleteInitConfiguration(
            self.instance_id, name)

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
            name = dialog.name
            get_network().onSaveInitConfiguration(
                name, [self.instance_id], update=True)

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
        name = model.index(index.row(), NAME_COLUMN).data()
        preview = self.ui_preview.isChecked()
        get_network().onGetInitConfiguration(
            self.instance_id, name, preview)

    @Slot()
    def on_ui_button_refresh_clicked(self):
        """Refresh the list of configurations for the actual `instance_id`"""
        get_network().onListInitConfigurations(self.instance_id)

    @Slot(str)
    def _filter_changed(self, text):
        self.model.setFilterFixedString(text)
        self._check_button_state()

    @Slot(QItemSelection, QItemSelection)
    def _selectionChanged(self, selected, deselected):
        self._check_button_state()


ConfigurationEntry = namedtuple("ConfigurationEntry", ["name", "timestamp"])


class TableModel(QAbstractTableModel):
    headers = ["Name", "Timestamp"]

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
                    timestamp=utc_to_local(item["timepoint"]),
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

        if role in (Qt.DisplayRole, Qt.ToolTipRole):
            entry = self.data[index.row()]
            column = index.column()
            return entry[column]

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]
