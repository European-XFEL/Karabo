#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from collections import OrderedDict, namedtuple
from operator import attrgetter
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, QAbstractTableModel, Qt
from PyQt4.QtGui import QDialog, QDialogButtonBox, QItemSelectionModel

from karabo_gui.events import (
    register_for_broadcasts, unregister_from_broadcasts, KaraboEventSender,
)
from karabo_gui.singletons.api import get_db_conn
from karabo_gui.util import SignalBlocker

SIMPLE_NAME = 'simple_name'
UUID = 'uuid'
AUTHOR = 'author'
PUBLISHED = 'published'
DESCRIPTION = 'description'
DOCUMENTATION = 'documentation'

PROJECT_DATA = OrderedDict()
PROJECT_DATA[SIMPLE_NAME] = 'Name'
PROJECT_DATA[UUID] = 'UUID'
PROJECT_DATA[AUTHOR] = 'Author'
PROJECT_DATA[PUBLISHED] = 'Published'
PROJECT_DATA[DESCRIPTION] = 'Description'
PROJECT_DATA[DOCUMENTATION] = 'Documentation'
ProjectEntry = namedtuple('ProjectEntry', [key for key in PROJECT_DATA.keys()])


def get_column_index(project_data_key):
    """ Return ``index`` position in ``PROJECT_DATA`` OrderedDict for the given
    ``project_data_key``.

    If the ``project_data_key`` is not found, ``None`` is returned."""
    return list(PROJECT_DATA.keys()).index(project_data_key)


class ProjectHandleDialog(QDialog):
    def __init__(self, simple_name, title, btn_text, parent=None):
        super(ProjectHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'project_handle.ui')
        uic.loadUi(filepath, self)

        self._set_dialog_texts(title, btn_text)

        # Tableview
        self.twProjects.setModel(TableModel(parent=self))
        self.twProjects.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.twProjects.doubleClicked.connect(self.accept)

        # Domain combobox
        db_conn = get_db_conn()
        self.default_domain = db_conn.default_domain
        self._domains_updated(db_conn.get_available_domains())

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.leTitle.textChanged.connect(self._titleChanged)
        self.leTitle.setText(simple_name)

        register_for_broadcasts(self)

    def done(self, result):
        """ Reimplement ``QDialog`` virtual slot

        Stop listening for broadcast events
        """
        unregister_from_broadcasts(self)
        super(ProjectHandleDialog, self).done(result)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.ProjectItemsList:
            items = data.get('items', [])
            self.twProjects.model().add_project_manager_data(items)
        elif sender is KaraboEventSender.ProjectDomainsList:
            self._domains_updated(data.get('items', []))
        return False

    def _domains_updated(self, domains):
        # Domain combobox
        with SignalBlocker(self.cbDomain):
            self.cbDomain.clear()
            self.cbDomain.addItems(sorted(domains))
        # Select default domain
        index = self.cbDomain.findText(self.default_domain)
        self.cbDomain.setCurrentIndex(index if index > -1 else 0)

    def _set_dialog_texts(self, title, btn_text):
        """ This method sets the ``title`` and the ``btn_text`` of the ok
        button.

        :param title: The new window title
        :param btn_text: The new text for ok button of the ``QDialogButtonBox``
        """
        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText(btn_text)

    def selected_item(self):
        """Return selected domain and project
        """
        selection_model = self.twProjects.selectionModel()
        uuid_index = get_column_index(UUID)
        uuid_entry = selection_model.selectedRows(uuid_index)
        if uuid_entry:
            return self.cbDomain.currentText(), uuid_entry[0].data()
        return None, None

    @property
    def simple_name(self):
        rows = self.twProjects.selectionModel().selectedRows()
        if rows:
            return rows[0].data()

    @pyqtSlot(object, object)
    def _selectionChanged(self, selected, deselected):
        """ Whenever an item is selected the current title and the button box
        need to be updated
        """
        rows = self.twProjects.selectionModel().selectedRows()
        enable = True if rows else False
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)

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
        enable = True if text else False
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)

    @pyqtSlot(str)
    def on_cbDomain_currentIndexChanged(self, domain):
        if domain:
            self.twProjects.model().request_data(domain)

    @pyqtSlot(bool)
    def on_cbShowTrash_toggled(self, is_checked):
        model = self.twProjects.model()
        model.show_trashed = is_checked
        model.request_data(self.cbDomain.currentText())


class NewProjectDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(NewProjectDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'project_new.ui')
        uic.loadUi(filepath, self)

        if model is None:
            title = 'New project'
        else:
            title = 'Edit project'
            self.leTitle.setText(model.simple_name)
        self.setWindowTitle(title)

    @property
    def simple_name(self):
        return self.leTitle.text()


class LoadProjectDialog(ProjectHandleDialog):
    def __init__(self, simple_name='', title="Load project", btn_text="Load",
                 parent=None):
        super(LoadProjectDialog, self).__init__(simple_name, title, btn_text,
                                                parent)


class SaveProjectDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(SaveProjectDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)), 'project_save.ui')
        uic.loadUi(filepath, self)

        # Domain combobox
        db_conn = get_db_conn()
        self.default_domain = db_conn.default_domain
        self._fill_domain_combo_box(db_conn.get_available_domains())

        self.setWindowTitle('Save {}'.format(model.simple_name))

        register_for_broadcasts(self)

    def done(self, result):
        """ Reimplement ``QDialog`` virtual slot

        Stop listening for broadcast events
        """
        unregister_from_broadcasts(self)
        super(SaveProjectDialog, self).done(result)

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

    def request_data(self, domain):
        """Request data for the given ``domain``
        """
        project_data = self.db_conn.get_available_project_data(
            domain, 'project')
        self.add_project_manager_data(project_data)

    def add_project_manager_data(self, data):
        """ Add the given `data` to the internal data structure

        :param data: A `HashList` with the keys per entry:
                     - 'uuid' - The unique ID of the Project
                     - 'simple_name' - The name for displaying
                     - 'item_type' - Should be project in that case
        """
        # XXX: this only works if the sent list of uuids is complete
        self.beginResetModel()
        try:
            self.entries = []
            for it in data:
                if (it.get('is_trashed') == 'true') != self.show_trashed:
                    continue
                entry = ProjectEntry(
                    simple_name=it.get('simple_name'),
                    uuid=it.get('uuid'),
                    author='',
                    published='',
                    description='description',
                    documentation='documentation',
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
        if role in (Qt.DisplayRole, Qt.ToolTipRole):
            return entry[index.column()]
        elif role == Qt.UserRole:
            return entry[index.column()]
        return None

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]

    def sort(self, column, order=Qt.AscendingOrder):
        """ Sort table by given column number and order """
        self.entries.sort(key=attrgetter(ProjectEntry._fields[column]),
                          reverse=bool(order))
        self.layoutChanged.emit()
        super(TableModel, self).sort(column, order)
