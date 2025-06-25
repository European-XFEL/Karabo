#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
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
#############################################################################
from collections import OrderedDict, namedtuple
from operator import attrgetter

from qtpy import uic
from qtpy.QtCore import (
    QAbstractTableModel, QItemSelection, QModelIndex, QSortFilterProxyModel,
    Qt, Slot)
from qtpy.QtWidgets import (
    QAbstractButton, QButtonGroup, QDialog, QDialogButtonBox)

from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.logger import get_logger
from karabogui.singletons.api import get_config, get_db_conn
from karabogui.util import InputValidator, SignalBlocker, utc_to_local

from .utils import get_dialog_ui

SIMPLE_NAME = "simple_name"
LAST_MODIFIED = "last_modified"
UUID = "uuid"

PROJECT_DATA = OrderedDict()
PROJECT_DATA[SIMPLE_NAME] = "Name"
PROJECT_DATA[LAST_MODIFIED] = "Last Modified"
PROJECT_DATA[UUID] = "UUID"

ProjectEntry = namedtuple("ProjectEntry", list(PROJECT_DATA))
HEADER = list(PROJECT_DATA.values())


def get_column_index(project_data_key):
    """Return `index` in ``PROJECT_DATA`` for `project_data_key`"""
    return list(PROJECT_DATA.keys()).index(project_data_key)


class LoadProjectDialog(QDialog):
    def __init__(self, is_subproject=False, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("project_handle.ui"), self)
        # set proper window flags
        self.setWindowFlags(self.windowFlags() | Qt.WindowCloseButtonHint)

        db_conn = get_db_conn()
        self.rbFromRemote.setChecked(db_conn.ignore_local_cache)
        self.rbFromCache.setChecked(not db_conn.ignore_local_cache)
        self.load_from_group = QButtonGroup(self)
        self.load_from_group.addButton(self.rbFromRemote)
        self.load_from_group.addButton(self.rbFromCache)
        self.load_from_group.buttonClicked.connect(self._open_from_changed)

        if is_subproject:
            title = "Load Sub Project"
        else:
            title = "Load Master Project"
        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText("Load")

        self.model = QSortFilterProxyModel(parent=self)
        # Set up the filter model!
        self.model.setSourceModel(TableModel(parent=self))
        self.model.setFilterRole(Qt.DisplayRole)
        self.model.setFilterFixedString("")
        self.model.setFilterCaseSensitivity(False)
        self.model.setFilterKeyColumn(0)

        column = int(get_config()["project_sort_column"])
        order = int(get_config()["project_sort_order"])
        self.twProjects.horizontalHeader().setSortIndicator(column, order)

        # QTableview in ui file
        self.twProjects.setModel(self.model)
        self.twProjects.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.twProjects.doubleClicked.connect(self._load_item)
        self.twProjects.horizontalHeader().sortIndicatorChanged.connect(
            self._sorting_changed)
        # Domain is not selectable for subprojects - only master projects
        self.cbDomain.setEnabled(not is_subproject)
        # ... request the domains list
        # initial request
        self.initial_request = False
        domains = db_conn.get_available_domains()

        # Domain combobox
        if is_subproject:
            default_domain = db_conn.default_domain
        else:
            topic = get_config()["broker_topic"]
            default_domain = (topic if topic in domains
                              else db_conn.default_domain)
        self.default_domain = default_domain

        if not self.ignore_cache:
            # Only fill with the cache domains if the user has requested it!
            self._domains_updated(domains)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.leTitle.textChanged.connect(self._titleChanged)
        self.pbClear.clicked.connect(self.leTitle.clear)

        self.event_map = {
            KaraboEvent.ProjectItemsList: self._event_item_list,
            KaraboEvent.ProjectDomainsList: self._event_domain_list,
            KaraboEvent.ProjectTrashed: self._event_trashed
        }
        register_for_broadcasts(self.event_map)

        # Set the focus on the search title!
        self.leTitle.setFocus()

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_item_list(self, data):
        items = data.get("items", [])
        self.model.sourceModel().add_project_manager_data(items)
        # NOTE: Resize all columns until qtpy
        self.twProjects.resizeColumnsToContents()
        self._titleChanged(self.leTitle.text())

    def _event_domain_list(self, data):
        self._domains_updated(data.get("items", []))

    def _event_trashed(self, data):
        domain = data.get("domain", "")
        self.on_cbDomain_currentIndexChanged(domain)

    # -----------------------------------------------------------------------

    def done(self, result):
        """ Reimplement ``QDialog`` virtual slot

        Stop listening for broadcast events
        """
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    def _domains_updated(self, domains):
        # Domain combobox
        with SignalBlocker(self.cbDomain):
            self.cbDomain.clear()
            self.cbDomain.addItems(sorted(domains))
        # Select default domain, can be initially `None`
        index = 0
        if self.default_domain is not None:
            index = self.cbDomain.findText(self.default_domain)
        with SignalBlocker(self.cbDomain):
            self.cbDomain.setCurrentIndex(index if index > -1 else 0)
        if not self.initial_request:
            self.initial_request = True
            self.update_view()

    def selected_item(self):
        """Return selected domain and project
        """
        selection_model = self.twProjects.selectionModel()
        uuid_index = get_column_index(UUID)
        uuid_entry = selection_model.selectedRows(uuid_index)
        if uuid_entry:
            return self.domain, uuid_entry[0].data()
        return None, None

    def update_view(self):
        if self.domain:
            self.twProjects.clearSelection()
            model = self.model.sourceModel()
            model.request_data(self.domain, self.ignore_cache)

    @property
    def ignore_cache(self):
        return self.rbFromRemote.isChecked()

    @property
    def simple_name(self):
        rows = self.twProjects.selectionModel().selectedRows()
        if rows:
            return rows[0].data()

    @property
    def domain(self):
        return self.cbDomain.currentText()

    def _selected_item_loadable(self):
        """ Return whether the currently selected is project loadable.
        """
        col_index = get_column_index(UUID)
        if self.twProjects.selectionModel().selectedRows(col_index):
            return True

        return False

    def _check_button_state(self):
        # Check if we have a preceeding valid selection
        enable = self._selected_item_loadable()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)

    @Slot(int, Qt.SortOrder)
    def _sorting_changed(self, column, order):
        """Change the view when sorting the table"""
        get_config()["project_sort_column"] = column
        get_config()["project_sort_order"] = order
        self.update_view()

    @Slot(str)
    def _titleChanged(self, text):
        self.model.setFilterFixedString(text)
        self._check_button_state()

    @Slot(QItemSelection, QItemSelection)
    def _selectionChanged(self, selected, deselected):
        """ Whenever an item is selected the current title and the button box
        need to be updated
        """
        # Make sure loading of trashed projects is not possible
        self._check_button_state()

    @Slot(QModelIndex)
    def _load_item(self, index):
        """ Slot connectect to the ``QTableView`` signal ``doubleClicked``
        Only accept the dialog, if the selected project is loadable.
        """
        # Make sure loading of trashed projects is not possible
        if self._selected_item_loadable():
            index = index.sibling(index.row(), 0)
            simple_name = self.model.data(index, Qt.DisplayRole)
            text = (f"Loading project <b>{simple_name}</b> from project "
                    "database")
            get_logger().info(text)
            self.accept()

    @Slot(str)
    def on_cbDomain_currentIndexChanged(self, domain):
        self.update_view()

    @Slot(bool)
    def on_cbShowTrash_toggled(self, is_checked):
        model = self.model.sourceModel()
        model.show_trashed = is_checked

        self._check_button_state()
        self.update_view()

    @Slot(QAbstractButton)
    def _open_from_changed(self, button):
        self.update_view()


class NewProjectDialog(QDialog):
    def __init__(self, model=None, is_rename=False, default=False,
                 parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("project_new.ui"), self)
        self.setModal(False)

        validator = InputValidator()

        # Domain combobox
        db_conn = get_db_conn()
        domains = db_conn.get_available_domains()
        if default:
            default_domain = db_conn.default_domain
        else:
            topic = get_config()["broker_topic"]
            default_domain = (topic if topic in domains
                              else db_conn.default_domain)
        self.default_domain = default_domain
        self._fill_domain_combo_box(domains)
        # Subprojects reside in the domain of the parent project, only allow
        # the default!
        self.cbDomain.setEnabled(not default)

        if model is None:
            title = "New project"
        else:
            if is_rename:
                title = "Rename project"
                text = model.simple_name
                # Hide domain related widgets
                self.laDomain.hide()
                self.cbDomain.hide()
                self.adjustSize()
            else:
                title = "Create a copy of this project..."
                text = f"{model.simple_name}_copy"
            self.leTitle.setText(text)
        self.setWindowTitle(title)
        self.leTitle.setFocus()
        self.leTitle.selectAll()
        self.leTitle.setValidator(validator)
        self.leTitle.textChanged.connect(self.validate)
        self.validate()

        self.event_map = {
            KaraboEvent.ProjectDomainsList: self._event_item_list
        }
        register_for_broadcasts(self.event_map)

    # -----------------------------------------------------------------------
    # PyQt Slots

    @Slot()
    def validate(self):
        enabled = self.leTitle.hasAcceptableInput()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_item_list(self, data):
        self._fill_domain_combo_box(data.get("items", []))

    # -----------------------------------------------------------------------

    def done(self, result):
        """ Reimplement ``QDialog`` virtual slot

        Stop listening for broadcast events
        """
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    @property
    def simple_name(self):
        return self.leTitle.text()

    @property
    def domain(self):
        return self.cbDomain.currentText()

    def _fill_domain_combo_box(self, domains):
        """Put the given list of ``domains`` into the given ``domain_combo``
        """
        with SignalBlocker(self.cbDomain):
            self.cbDomain.clear()
            self.cbDomain.addItems(sorted(domains))
        # Select default domain
        index = self.cbDomain.findText(self.default_domain)
        self.cbDomain.setCurrentIndex(index if index > -1 else 0)


class TableModel(QAbstractTableModel):
    headers = HEADER

    def __init__(self, show_trashed=False, parent=None):
        super().__init__(parent)
        self.entries = []
        self.db_conn = get_db_conn()
        self.show_trashed = show_trashed

    def request_data(self, domain, ignore_cache):
        """Request data for the given ``domain``
        """
        self.db_conn.ignore_local_cache = ignore_cache
        project_data = self.db_conn.get_available_project_data(domain,
                                                               "project")
        self.add_project_manager_data(project_data)

    def add_project_manager_data(self, data):
        """ Add the given `data` to the internal data structure

        :param data: A `HashList` with the keys per entry:
                     - 'uuid' - The unique ID of the Project
                     - 'simple_name' - The name for displaying
                     - 'is_trashed' - Flag if project is marked as trashed
                     - 'date' - The date the project was last modified

        Note: The `user` information is not used.
        """
        # XXX: this only works if the sent list of uuids is complete
        self.beginResetModel()
        try:
            self.entries = []
            for it in data:
                is_trashed = it.get("is_trashed")
                if is_trashed != self.show_trashed:
                    continue
                entry = ProjectEntry(
                    simple_name=it.get("simple_name"),
                    last_modified=utc_to_local(it.get("date")),
                    uuid=(it.get("uuid"), is_trashed))
                self.entries.append(entry)
        finally:
            self.endResetModel()

        # Sort the table according to the column
        column = int(get_config()["project_sort_column"])
        order = int(get_config()["project_sort_order"])
        self.sort(column, order)

    def rowCount(self, parent=None):
        return len(self.entries)

    def columnCount(self, parent=None):
        return len(self.headers)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        entry = self.entries[index.row()]
        column = index.column()
        if role == Qt.DisplayRole:
            if column == get_column_index(UUID):
                # Only display uuid here, ignore is_trashed
                uuid, _ = entry[column]
                return uuid
            return entry[column]
        elif role == Qt.UserRole:
            return entry[column]
        return None

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]

    def sort(self, column, order=Qt.AscendingOrder):
        """ Sort table by given column number and order """
        self.layoutAboutToBeChanged.emit()
        self.entries.sort(key=attrgetter(ProjectEntry._fields[column]),
                          reverse=bool(order))
        self.layoutChanged.emit()
        super().sort(column, order)
