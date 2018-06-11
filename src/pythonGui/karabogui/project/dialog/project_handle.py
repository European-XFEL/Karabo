#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict, namedtuple
from functools import partial
from operator import attrgetter
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, QAbstractTableModel, Qt
from PyQt4.QtGui import (QAction, QButtonGroup, QCursor, QDialog,
                         QDialogButtonBox, QHeaderView, QItemSelectionModel,
                         QMenu)

from karabogui import messagebox
from karabogui.events import (
    register_for_broadcasts, unregister_from_broadcasts, KaraboEventSender,
)
from karabogui.project.utils import show_trash_project_message
from karabogui.singletons.api import get_db_conn
from karabogui.util import SignalBlocker, utc_to_local

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

        # QTableview in ui file
        self.twProjects.horizontalHeader().setResizeMode(
            QHeaderView.ResizeToContents)
        self.twProjects.setModel(TableModel(parent=self))
        self.twProjects.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.twProjects.doubleClicked.connect(self._load_item)
        self.twProjects.setContextMenuPolicy(Qt.CustomContextMenu)
        self.twProjects.customContextMenuRequested.connect(
            self._show_context_menu)

        # Domain is not selectable for subprojects - only master projects
        self.cbDomain.setEnabled(not is_subproject)
        # Domain combobox
        self.default_domain = db_conn.default_domain
        # ... request the domains list
        domains = db_conn.get_available_domains()
        if not self.ignore_cache:
            # Only fill with the cache domains if the user has requested it!
            self._domains_updated(domains)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.leTitle.textChanged.connect(self._titleChanged)

        register_for_broadcasts(self)

    def done(self, result):
        """ Reimplement ``QDialog`` virtual slot

        Stop listening for broadcast events
        """
        unregister_from_broadcasts(self)
        super(LoadProjectDialog, self).done(result)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.ProjectItemsList:
            items = data.get('items', [])
            self.twProjects.model().add_project_manager_data(items)
            # Match only the simple name column to content
            self.twProjects.resizeColumnToContents(0)
            return True
        elif sender is KaraboEventSender.ProjectDomainsList:
            self._domains_updated(data.get('items', []))
            return True
        elif sender is KaraboEventSender.ProjectAttributeUpdated:
            self._is_trashed_updated(data.get('items', []))
            return True
        return False

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
                                    modal=False)
        if index in (self.cbDomain.currentIndex(), -1):
            # Make sure the signal is triggered when setting the index below
            self.cbDomain.setCurrentIndex(-1)
        self.cbDomain.setCurrentIndex(index if index > -1 else 0)

    def _is_trashed_updated(self, items):
        for it in items:
            if (it.get('attr_name') != 'is_trashed'
                    and it.get('item_type') != 'project'):
                continue
            if not it.get('success', True):
                messagebox.show_error(it['reason'])
                break
            domain = it.get('domain')
            self.on_cbDomain_currentIndexChanged(domain)

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
            self.twProjects.model().request_data(self.domain,
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
        """ Return whether the currently selected project loadable.
        """
        col_index = get_column_index(UUID)
        rows = self.twProjects.selectionModel().selectedRows(col_index)
        if rows:
            _, is_trashed = rows[0].data(Qt.UserRole)
            return not is_trashed

        return False

    def _text_item_loadable(self, simple_name):
        """ Return whether the entered project name is in the project table.
        """
        entries = self.twProjects.model().entries
        projects = [entry.simple_name for entry in entries]
        return simple_name in projects

    def _check_button_state(self):
        # Check if we have a preceeding selection
        selectable = self._selected_item_loadable()
        # If we are typing a project name
        simple_name = self.leTitle.text()
        project = len(simple_name) and self._text_item_loadable(simple_name)
        trash = self.cbShowTrash.isChecked()

        enable = selectable or (project and not trash)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)

    @pyqtSlot(object, object)
    def _selectionChanged(self, selected, deselected):
        """ Whenever an item is selected the current title and the button box
        need to be updated
        """
        # Make sure loading of trashed projects is not possible
        self._check_button_state()

    @pyqtSlot(object)
    def _load_item(self, index):
        """ Slot connectect to the ``QTableView`` signal ``doubleClicked``
        Only accept the dialog, if the selected project is loadable.
        """
        # Make sure loading of trashed projects is not possible
        if self._selected_item_loadable():
            self.accept()

    @pyqtSlot(object)
    def _titleChanged(self, text):
        index = self.twProjects.model().projectIndex(text)
        with SignalBlocker(self.twProjects):
            if index is not None:
                selection_flag = (QItemSelectionModel.ClearAndSelect
                                  | QItemSelectionModel.Rows)
                self.twProjects.selectionModel().setCurrentIndex(
                    index, selection_flag)
            else:
                self.twProjects.selectionModel().clearSelection()

        self._check_button_state()

    @pyqtSlot(str)
    def on_cbDomain_currentIndexChanged(self, domain):
        self.update_view()

    @pyqtSlot(bool)
    def on_cbShowTrash_toggled(self, is_checked):
        model = self.twProjects.model()
        model.show_trashed = is_checked

        self._check_button_state()
        self.update_view()

    @pyqtSlot()
    def _show_context_menu(self):
        """ Show a context menu for the currently selected project
        """
        selection_model = self.twProjects.selectionModel()
        # Get all indexes for selected row (single selection)
        indexes = selection_model.selectedIndexes()
        if indexes:
            col_index = get_column_index(UUID)
            uuid, is_trashed = indexes[col_index].data(Qt.UserRole)
            if is_trashed:
                text = 'Restore from trash'
            else:
                text = 'Move to trash'
            menu = QMenu(self)
            trash_action = QAction(text, menu)
            trash_action.triggered.connect(partial(self._update_is_trashed,
                                                   self.domain, uuid,
                                                   is_trashed))
            menu.addAction(trash_action)
            menu.exec(QCursor.pos())

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
            self.update_view()

    @pyqtSlot(object)
    def _openFromChanged(self, button):
        # Update view
        self.update_view()


class NewProjectDialog(QDialog):
    def __init__(self, model=None, is_rename=False, parent=None):
        super(NewProjectDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'project_new.ui')
        uic.loadUi(filepath, self)

        # Domain combobox
        db_conn = get_db_conn()
        self.default_domain = db_conn.default_domain
        self._fill_domain_combo_box(db_conn.get_available_domains())

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

        register_for_broadcasts(self)

    def done(self, result):
        """ Reimplement ``QDialog`` virtual slot

        Stop listening for broadcast events
        """
        unregister_from_broadcasts(self)
        super(NewProjectDialog, self).done(result)

    @property
    def simple_name(self):
        return self.leTitle.text()

    def karaboBroadcastEvent(self, event):
        if event.sender is KaraboEventSender.ProjectDomainsList:
            self._fill_domain_combo_box(event.data.get('items', []))
        return False

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
