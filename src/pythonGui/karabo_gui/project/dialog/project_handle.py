#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op
from collections import OrderedDict, namedtuple

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, QAbstractTableModel, Qt
from PyQt4.QtGui import QDialog, QDialogButtonBox

from karabo_gui.util import SignalBlocker
from karabo_gui.mediator import (
    register_for_broadcasts, unregister_from_broadcasts, KaraboEventSender,
    KaraboBroadcastEvent
)
from karabo_gui.topology import Manager

PROJECT_DATA = OrderedDict()
PROJECT_DATA['uuid'] = 'Name'
PROJECT_DATA['author'] = 'Author'
PROJECT_DATA['revision'] = 'Version'
PROJECT_DATA['published'] = 'Published'
PROJECT_DATA['description'] = 'Description'
PROJECT_DATA['documentation'] = 'Documentation'
ProjectEntry = namedtuple('ProjectEntry', [key for key in PROJECT_DATA.keys()])


class ProjectHandleDialog(QDialog):
    def __init__(self, title, btn_text, parent=None):
        super(ProjectHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'project_handle.ui')
        uic.loadUi(filepath, self)

        self.set_dialog_texts(title, btn_text)
        self.buttonBox.accepted.connect(self.accept)

        self.twProjects.setModel(TableModel(self))
        self.twProjects.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.twProjects.doubleClicked.connect(self.accept)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.leTitle.textChanged.connect(self._titleChanged)

        register_for_broadcasts(self)

    def closeEvent(self, event):
        """Stop listening for broadcast events
        """
        unregister_from_broadcasts(self)
        event.accept()

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.ProjectItemsList:
                uuids = event.data.get('items', [])
                self.twProjects.model().add_project_manager_data(uuids)
        return False

    def set_dialog_texts(self, title, btn_text):
        """ This method sets the ``title`` and the ``btn_text`` of the ok
        button.

        :param title: The new window title
        :param btn_text: The new text for ok button of the ``QDialogButtonBox``
        """
        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText(btn_text)

    def selected_item(self):
        rows = self.twProjects.selectionModel().selectedRows()
        if rows:
            return rows[0].data()
        return None

    @property
    def simple_name(self):
        return self.leTitle.text()

    @pyqtSlot(object, object)
    def _selectionChanged(self, selected, deselected):
        """ Whenever an item is selected the current title and the button box
        need to be updated
        """
        rows = self.twProjects.selectionModel().selectedRows()
        if rows:
            with SignalBlocker(self.leTitle):
                self.leTitle.setText(rows[0].data())
        enable = True if rows else False
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)

    @pyqtSlot(object)
    def _titleChanged(self, text):
        if not self.twProjects.model().hasProject(text):
            with SignalBlocker(self.twProjects):
                self.twProjects.selectionModel().clearSelection()
        enable = True if text else False
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enable)


class NewDialog(ProjectHandleDialog):
    def __init__(self, title="New Project", btn_text="New", parent=None):
        super(NewDialog, self).__init__(title, btn_text, parent)


class LoadDialog(ProjectHandleDialog):
    def __init__(self, title="Load Project", btn_text="Load", parent=None):
        super(LoadDialog, self).__init__(title, btn_text, parent)


class SaveDialog(ProjectHandleDialog):
    def __init__(self, title="Save Project", btn_text="Save", parent=None):
        super(SaveDialog, self).__init__(title, btn_text, parent)


class TableModel(QAbstractTableModel):
    headers = [value for value in PROJECT_DATA.values()]

    def __init__(self, parent=None):
        super(TableModel, self).__init__(parent)
        self.entries = []
        self._extractData()

    def _extractData(self):
        from karabo_gui.project.api import TEST_DOMAIN
        db_conn = Manager().proj_db_conn
        project_uuids = db_conn.get_uuids_of_type(TEST_DOMAIN, 'project')
        self.add_project_manager_data(project_uuids)

    def add_project_manager_data(self, uuids):
        for uuid in uuids:
            # XXX: Fetch the other information via ``uuid``
            entry = ProjectEntry(
                uuid=uuid,
                author='author',
                revision='revision',
                published='published',
                description='description',
                documentation='documentation',
                )
            self.entries.append(entry)

    def hasProject(self, uuid):
        """ Check whether the given `uuid exists in the current model.
        :return: True if it exists, false otherwise
        """
        for entry in self.entries:
            if entry.uuid == uuid:
                return True
        return False

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
        return None

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]
