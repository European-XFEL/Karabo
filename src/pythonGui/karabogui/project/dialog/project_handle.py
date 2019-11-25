#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict, namedtuple
from operator import attrgetter
import os.path as op

from PyQt5 import uic
from PyQt5.QtCore import (
    pyqtSlot, QAbstractTableModel, QItemSelection, QModelIndex,
    QSortFilterProxyModel, Qt)
from PyQt5.QtWidgets import (
    QAbstractButton, QButtonGroup, QDialog, QDialogButtonBox)

from karabogui import messagebox
from karabogui.events import (
    register_for_broadcasts, unregister_from_broadcasts, KaraboEvent,
)
from karabogui.project.utils import show_trash_project_message
from karabogui.singletons.api import get_config, get_db_conn
from karabogui.util import InputValidator, SignalBlocker, utc_to_local

SIMPLE_NAME = 'simple_name'
LAST_MODIFIED = 'last_modified'
UUID = 'uuid'
AUTHOR = 'author'
DESCRIPTION = 'description'

PROJECT_DATA = OrderedDict()
PROJECT_DATA[SIMPLE_NAME] = 'Name'
PROJECT_DATA[LAST_MODIFIED] = 'Last Modified'
PROJECT_DATA[UUID] = 'UUID'
PROJECT_DATA[AUTHOR] = 'Author'
PROJECT_DATA[DESCRIPTION] = 'Description'
ProjectEntry = namedtuple('ProjectEntry', [key for key in PROJECT_DATA.keys()])


def get_column_index(project_data_key):
    """ Return ``index`` position in ``PROJECT_DATA`` OrderedDict for the given
    ``project_data_key``.

    If the ``project_data_key`` is not found, ``None`` is returned."""
    return list(PROJECT_DATA.keys()).index(project_data_key)


class LoadProjectDialog(QDialog):
    def __init__(self, is_subproject=False, parent=None):
        super(LoadProjectDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'project_handle.ui')
        uic.loadUi(filepath, self)
        # set proper window flags
        self.setWindowFlags(Qt.WindowCloseButtonHint)

        db_conn = get_db_conn()
        self.rbFromRemote.setChecked(db_conn.ignore_local_cache)
        self.rbFromCache.setChecked(not db_conn.ignore_local_cache)
        self.load_from_group = QButtonGroup(self)
        self.load_from_group.addButton(self.rbFromRemote)
        self.load_from_group.addButton(self.rbFromCache)
        self.load_from_group.buttonClicked.connect(self._openFromChanged)

        if is_subproject:
            title = 'Load Sub Project'
        else:
            title = 'Load Master Project'
        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText('Load')

        self.model = QSortFilterProxyModel(parent=self)
        # Set up the filter model!
        self.model.setSourceModel(TableModel(parent=self))
        self.model.setFilterRole(Qt.DisplayRole)
        self.model.setFilterFixedString("")
        self.model.setFilterCaseSensitivity(False)
        self.model.setFilterKeyColumn(0)

        # QTableview in ui file
        self.twProjects.setModel(self.model)
        self.twProjects.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.twProjects.doubleClicked.connect(self._load_item)

        # Domain is not selectable for subprojects - only master projects
        self.cbDomain.setEnabled(not is_subproject)
        # ... request the domains list
        domains = db_conn.get_available_domains()

        # Domain combobox
        if is_subproject:
            default_domain = db_conn.default_domain
        else:
            topic = get_config()['broker_topic']
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
            KaraboEvent.ProjectAttributeUpdated: self._event_attribute
        }
        register_for_broadcasts(self.event_map)

        # Set the focus on the search title!
        self.leTitle.setFocus()

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_item_list(self, data):
        items = data.get('items', [])
        self.model.sourceModel().add_project_manager_data(items)
        # NOTE: Resize all columns until PyQt5
        self.twProjects.resizeColumnsToContents()
        self._titleChanged(self.leTitle.text())

    def _event_domain_list(self, data):
        self._domains_updated(data.get('items', []))

    def _event_attribute(self, data):
        items = data.get('items', [])
        for it in items:
            if (it.get('attr_name') != 'is_trashed'
                    and it.get('item_type') != 'project'):
                continue
            if not it.get('success', True):
                messagebox.show_error(it['reason'])
                break
            domain = it.get('domain')
            self.on_cbDomain_currentIndexChanged(domain)

    # -----------------------------------------------------------------------

    def done(self, result):
        """ Reimplement ``QDialog`` virtual slot

        Stop listening for broadcast events
        """
        unregister_from_broadcasts(self.event_map)
        super(LoadProjectDialog, self).done(result)

    def _domains_updated(self, domains):
        # Domain combobox
        with SignalBlocker(self.cbDomain):
            self.cbDomain.clear()
            self.cbDomain.addItems(sorted(domains))
        # Select default domain
        index = self.cbDomain.findText(self.default_domain)
        if len(domains) > 0 and index == -1:
            msg = ('The default project domain <b>{}</b><br>does not exist in '
                   'the current project database.').format(self.default_domain)
            # NOTE: If this dialog is not modal, it can block the list of
            # domains arriving from the GUI server!
            messagebox.show_warning(msg, title='Default domain does not exist',
                                    parent=self)
        if index in (self.cbDomain.currentIndex(), -1):
            # Make sure the signal is triggered when setting the index below
            self.cbDomain.setCurrentIndex(-1)
        self.cbDomain.setCurrentIndex(index if index > -1 else 0)

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
            self.model.sourceModel().request_data(self.domain,
                                                  self.ignore_cache)

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

    @pyqtSlot(str)
    def _titleChanged(self, text):
        self.model.setFilterFixedString(text)
        self._check_button_state()

    @pyqtSlot(QItemSelection, QItemSelection)
    def _selectionChanged(self, selected, deselected):
        """ Whenever an item is selected the current title and the button box
        need to be updated
        """
        # Make sure loading of trashed projects is not possible
        self._check_button_state()

    @pyqtSlot(QModelIndex)
    def _load_item(self, index):
        """ Slot connectect to the ``QTableView`` signal ``doubleClicked``
        Only accept the dialog, if the selected project is loadable.
        """
        # Make sure loading of trashed projects is not possible
        if self._selected_item_loadable():
            self.accept()

    @pyqtSlot(str)
    def on_cbDomain_currentIndexChanged(self, domain):
        self.update_view()

    @pyqtSlot(bool)
    def on_cbShowTrash_toggled(self, is_checked):
        model = self.model.sourceModel()
        model.show_trashed = is_checked

        self._check_button_state()
        self.update_view()

    @pyqtSlot(str, str, bool)
    def _update_is_trashed(self, domain, uuid, current_is_trashed):
        """ Change ``is_trashed`` attribute of project with given ``uuid``

        NOTE: ``current_is_trashed`` is the current value of the selected
        project which should be toggled here
        """
        if show_trash_project_message(current_is_trashed):
            db_conn = get_db_conn()
            db_conn.update_attribute(domain, 'project', uuid, 'is_trashed',
                                     str(not current_is_trashed).lower())
            # NOTE: The view update is happening asynchronously. Once we get
            # a reply from the GUI server, we request a new view

    @pyqtSlot(QAbstractButton)
    def _openFromChanged(self, button):
        # Update view
        self.update_view()


class NewProjectDialog(QDialog):
    def __init__(self, model=None, is_rename=False, default=False,
                 parent=None):
        super(NewProjectDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'project_new.ui')
        uic.loadUi(filepath, self)
        self.setModal(False)

        validator = InputValidator()

        # Domain combobox
        db_conn = get_db_conn()
        domains = db_conn.get_available_domains()
        if default:
            default_domain = db_conn.default_domain
        else:
            topic = get_config()['broker_topic']
            default_domain = (topic if topic in domains
                              else db_conn.default_domain)
        self.default_domain = default_domain
        self._fill_domain_combo_box(domains)
        # Subprojects reside in the domain of the parent project, only allow
        # the default!
        self.cbDomain.setEnabled(not default)

        if model is None:
            title = 'New project'
        else:
            if is_rename:
                title = 'Rename project'
                text = model.simple_name
                # Hide domain related widgets
                self.laDomain.hide()
                self.cbDomain.hide()
                self.adjustSize()
            else:
                title = 'Create a copy of this project...'
                text = '{}_copy'.format(model.simple_name)
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

    @pyqtSlot()
    def validate(self):
        enabled = self.leTitle.hasAcceptableInput()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_item_list(self, data):
        self._fill_domain_combo_box(data.get('items', []))

    # -----------------------------------------------------------------------

    def done(self, result):
        """ Reimplement ``QDialog`` virtual slot

        Stop listening for broadcast events
        """
        unregister_from_broadcasts(self.event_map)
        super(NewProjectDialog, self).done(result)

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
    headers = [value for value in PROJECT_DATA.values()]

    def __init__(self, show_trashed=False, parent=None):
        super(TableModel, self).__init__(parent)
        self.entries = []
        self.db_conn = get_db_conn()
        self.show_trashed = show_trashed

    def request_data(self, domain, ignore_cache):
        """Request data for the given ``domain``
        """
        self.db_conn.ignore_local_cache = ignore_cache
        project_data = self.db_conn.get_available_project_data(domain,
                                                               'project')
        self.add_project_manager_data(project_data)

    def add_project_manager_data(self, data):
        """ Add the given `data` to the internal data structure

        :param data: A `HashList` with the keys per entry:
                     - 'uuid' - The unique ID of the Project
                     - 'simple_name' - The name for displaying
                     - 'item_type' - Should be project in that case
                     - 'is_trashed' - Flag if project is marked as trashed
                     - 'user' - The user who created the project
                     - 'date' - The date the project was last modified
                     - 'description' - The description of the project
        """
        # XXX: this only works if the sent list of uuids is complete
        self.beginResetModel()
        try:
            self.entries = []
            for it in data:
                is_trashed = (it.get('is_trashed') == 'true')
                if is_trashed != self.show_trashed:
                    continue
                entry = ProjectEntry(
                    simple_name=it.get('simple_name'),
                    last_modified=utc_to_local(it.get('date')),
                    uuid=(it.get('uuid'), is_trashed),
                    author=it.get('user', ''),
                    description=it.get('description', ''),
                    )
                self.entries.append(entry)
        finally:
            self.endResetModel()
        # Sort by simple name when table got filled
        self.sort(get_column_index(SIMPLE_NAME))

    def projectIndex(self, simple_name):
        """Return the `QModelIndex` which ``simple_name`` exists in the current
        model

        :return: A ``QModelIndex`` which was found, otherwise a ``NoneType``
        returned
        """
        if simple_name:
            for index, entry in enumerate(self.entries):
                if entry.simple_name.startswith(simple_name):
                    return self.index(index, get_column_index(SIMPLE_NAME))

    def rowCount(self, parent=None):
        return len(self.entries)

    def columnCount(self, parent=None):
        return len(self.headers)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        entry = self.entries[index.row()]
        column = index.column()
        if role in (Qt.DisplayRole, Qt.ToolTipRole):
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
        super(TableModel, self).sort(column, order)
