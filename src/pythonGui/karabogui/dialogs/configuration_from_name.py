from collections import namedtuple
import os.path as op

from PyQt5 import uic
from PyQt5.QtCore import (
    pyqtSlot, QAbstractTableModel, QItemSelection, QSortFilterProxyModel, Qt)
from PyQt5.QtWidgets import QDialog, QHeaderView, QDialogButtonBox

from karabogui.events import (
    register_for_broadcasts, unregister_from_broadcasts, KaraboEvent)
from karabogui.singletons.api import get_network

NAME_FIELD = 0
DESCRIPTION_FIELD = 4


class SaveDialog(QDialog):
    """Save dialog for configuration details"""

    def __init__(self, parent=None):
        super(SaveDialog, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), "config_save.ui"), self)

        self.event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(self.event_map)

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    def done(self, result):
        """Stop listening for broadcast events"""
        unregister_from_broadcasts(self.event_map)
        super(SaveDialog, self).done(result)

    @property
    def description(self):
        return self.ui_description.toPlainText()

    @property
    def priority(self):
        return self.ui_priority.value()

    @property
    def name(self):
        return self.ui_name.text()


class ListConfigurationDialog(QDialog):
    """List configurations by ``name`` from the configuration database
    """

    def __init__(self, instance_id, parent=None):
        super(ListConfigurationDialog, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), "config_handle.ui"), self)
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
        header.setResizeMode(QHeaderView.ResizeToContents)
        header.setStretchLastSection(True)
        self.ui_table_widget.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.ui_table_widget.doubleClicked.connect(self._request_configuration)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.ui_filter.textChanged.connect(self._filter_changed)
        self.ui_button_clear.clicked.connect(self.ui_filter.clear)

        self.ui_filter.setFocus()

        # Provide saving option
        self.ui_button_save.clicked.connect(self.open_save_dialog)

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
        super(ListConfigurationDialog, self).done(result)

    def _check_existing(self):
        if self.ui_table_widget.selectionModel().hasSelection():
            return True

        return False

    def _check_button_state(self):
        enable = self._check_existing()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)

    # --------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot()
    def open_save_dialog(self):
        dialog = SaveDialog(parent=self)
        if dialog.exec() == QDialog.Accepted:
            priority = dialog.priority
            description = dialog.description
            name = dialog.name
            get_network().onSaveConfigurationFromName(
                name, [self.instance_id], priority=priority,
                description=description, update=True)

    @pyqtSlot()
    def accept(self):
        """The dialog was accepted and we can request a configuration"""
        self._request_configuration()
        super(ListConfigurationDialog, self).accept()

    @pyqtSlot()
    def _request_configuration(self):
        """Request the configuration for the actual `instance_id`"""
        index = self.ui_table_widget.selectionModel().selectedRows()[0]
        if not index.isValid():
            return
        model = index.model()
        name = model.index(index.row(), NAME_FIELD).data()
        get_network().onGetConfigurationFromName(self.instance_id, name)

    @pyqtSlot()
    def on_ui_button_refresh_clicked(self):
        """Refresh the list of configurations for the actual `instance_id`"""
        get_network().onListConfigurationFromName(self.instance_id)

    @pyqtSlot(str)
    def _filter_changed(self, text):
        self.model.setFilterFixedString(text)
        self._check_button_state()

    @pyqtSlot(QItemSelection, QItemSelection)
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
        super(TableModel, self).__init__(parent)
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
            return entry[column]
        elif role == Qt.ToolTipRole:
            return entry[DESCRIPTION_FIELD]
        elif role == Qt.UserRole:
            return entry[column]
        return None

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]