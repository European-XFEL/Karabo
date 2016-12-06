#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op
from collections import OrderedDict, namedtuple

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, QAbstractTableModel, Qt
from PyQt4.QtGui import (QComboBox, QDialog, QDialogButtonBox, QPixmap, QStyle,
                         QStyledItemDelegate)

from karabo_gui.events import (
    register_for_broadcasts, unregister_from_broadcasts, KaraboEventSender,
    KaraboBroadcastEvent
)
from karabo_gui.singletons.api import get_db_conn
from karabo_gui.util import SignalBlocker

SIMPLE_NAME = 'simple_name'
UUID = 'uuid'
AUTHOR = 'author'
REVISIONS = 'revisions'
PUBLISHED = 'published'
DESCRIPTION = 'description'
DOCUMENTATION = 'documentation'

PROJECT_DATA = OrderedDict()
PROJECT_DATA[SIMPLE_NAME] = 'Name'
PROJECT_DATA[UUID] = 'UUID'
PROJECT_DATA[AUTHOR] = 'Author'
PROJECT_DATA[REVISIONS] = 'Version'
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

        self.set_dialog_texts(title, btn_text)

        self.twProjects.setModel(TableModel(parent=self))
        rev_delegate = ComboBoxDelegate(self.twProjects)
        rev_column = get_column_index(REVISIONS)
        self.twProjects.setItemDelegateForColumn(rev_column, rev_delegate)
        self.twProjects.selectionModel().selectionChanged.connect(
            self._selectionChanged)
        self.twProjects.doubleClicked.connect(self.accept)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.leTitle.textChanged.connect(self._titleChanged)
        self.leTitle.setText(simple_name)

        register_for_broadcasts(self)

    def closeEvent(self, event):
        """Stop listening for broadcast events
        """
        unregister_from_broadcasts(self)
        event.accept()

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.ProjectItemsList:
                items = event.data.get('items', [])
                self.twProjects.model().add_project_manager_data(items)
            return False
        return super(ProjectHandleDialog, self).eventFilter(obj, event)

    def set_dialog_texts(self, title, btn_text):
        """ This method sets the ``title`` and the ``btn_text`` of the ok
        button.

        :param title: The new window title
        :param btn_text: The new text for ok button of the ``QDialogButtonBox``
        """
        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText(btn_text)

    def selected_item(self):
        selection_model = self.twProjects.selectionModel()
        uuid_index = get_column_index(UUID)
        uuid_entry = selection_model.selectedRows(uuid_index)
        rev_index = get_column_index(REVISIONS)
        rev_entry = selection_model.selectedRows(rev_index)
        if uuid_entry and rev_entry:
            delegate = self.twProjects.itemDelegate(rev_entry[0])
            return (uuid_entry[0].data(), int(delegate.selectedItem()))
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


class NewProjectDialog(QDialog):
    def __init__(self, title="New project", parent=None):
        super(NewProjectDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'project_new.ui')
        uic.loadUi(filepath, self)

        self.setWindowTitle(title)

    @property
    def simple_name(self):
        return self.leTitle.text()


class LoadProjectDialog(ProjectHandleDialog):
    def __init__(self, simple_name='', title="Load project", btn_text="Load",
                 parent=None):
        super(LoadProjectDialog, self).__init__(simple_name, title, btn_text,
                                                parent)


class TableModel(QAbstractTableModel):
    headers = [value for value in PROJECT_DATA.values()]

    def __init__(self, parent=None):
        super(TableModel, self).__init__(parent)
        self.entries = []
        self.db_conn = get_db_conn()
        self._extractData()

    def _extractData(self):
        from karabo_gui.project.api import TEST_DOMAIN

        project_data = self.db_conn.get_available_project_data(
            TEST_DOMAIN, 'project')
        self.add_project_manager_data(project_data)

    def add_project_manager_data(self, data):
        """ Add the given `data` to the internal data structure

        :param data: A `HashList` with the keys per entry:
                     - 'uuid' - The unique ID of the Project
                     - 'revisions' - A list of revisions for the given project
                     - 'simple_name' - The name for displaying
                     - 'item_type' - Should be project in that case
        """
        # XXX: this only works if the sent list of uuids is complete
        self.beginResetModel()
        try:
            self.entries = []
            for it in data:
                rev_list = it.get('revisions')
                rev_data = []
                for rev in rev_list:
                    rev_data.append((rev.get('revision', ''),
                                     rev.get('alias', '')))
                entry = ProjectEntry(
                    simple_name=it.get('simple_name'),
                    uuid=it.get('uuid'),
                    author=rev_list[0].get('user') if rev_list else '',
                    revisions=rev_data,
                    published=rev_list[0].get('date') if rev_list else '',
                    description='description',
                    documentation='documentation',
                    )
                self.entries.append(entry)
        finally:
            self.endResetModel()

    def hasProject(self, uuid):
        """ Check whether the given ``uuid`` exists in the current model.

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


class ComboBoxDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(ComboBoxDelegate, self).__init__(parent)
        # Fake combobox used for later rendering
        self.cbSelection = QComboBox(parent)
        self.cbSelection.hide()
        self.current_selection = 0
        parent.clicked.connect(self.cellClicked)
        self.cellEditMode = False
        self.currentCellIndex = None  # QPersistentModelIndex

    def selectedItem(self):
        return self.cbSelection.itemData(self.current_selection)

    def _isRelevantColumn(self, index):
        """ This methods checks whether the column of the given ``index``
            belongs to the revision column.

            Returns tuple:
            [0] - states whether this is a relevant column
            [1] - the list of revisions which needs to be shown in the combobox
            Otherwise ``False`` and an empty string is returned.
        """
        column = index.column()
        if column == get_column_index(REVISIONS):
            revisions = index.data()
            return (True, revisions)
        return (False, [])

    def _updateWidget(self, combo, index, revisions):
        """ Put given ``revisions`` in combobox

        :param combo: The `QComboBox` which should be updated
        :param index: A `QModelIndex` of the view
        :param revisions: A list with all available revisions and mapped aliases
                          as tuple
        """
        column = index.column()
        if column == get_column_index(REVISIONS):
            revisions = index.data()
            if not revisions:
                return
            with SignalBlocker(combo):
                combo.clear()
                for rev, alias in revisions:
                    combo.addItem('{} <{}>'.format(alias, rev), rev)

    def createEditor(self, parent, option, index):
        """ This method is called whenever the delegate is in edit mode."""
        isRelevant, revisions = self._isRelevantColumn(index)
        if isRelevant:
            # This combobox is for the highlighting effect when clicking/editing
            # the index, is deleted whenever `closePersistentEditor` is called
            combo = QComboBox(parent)
            combo.currentIndexChanged.connect(self.currentIndexChanged)
            self._updateWidget(combo, index, revisions)
            return combo
        else:
            return super(ComboBoxDelegate, self).createEditor(parent, option,
                                                              index)

    def setEditorData(self, combo, index):
        isRelevant, revisions = self._isRelevantColumn(index)
        if isRelevant:
            self._updateWidget(combo, index, revisions)
        else:
            super(ComboBoxDelegate, self).setEditorData(combo, index)

    def paint(self, painter, option, index):
        isRelevant, revisions = self._isRelevantColumn(index)
        if isRelevant:
            self.cbSelection.setGeometry(option.rect)
            self._updateWidget(self.cbSelection, index, revisions)
            if option.state == QStyle.State_Selected:
                painter.fillRect(option.rect, option.palette.highlight())
            pixmap = QPixmap.grabWidget(self.cbSelection)
            painter.drawPixmap(option.rect.x(), option.rect.y(), pixmap)
        else:
            super(ComboBoxDelegate, self).paint(painter, option, index)

    def updateEditorGeometry(self, combo, option, index):
        isRelevant, revisions = self._isRelevantColumn(index)
        if isRelevant:
            combo.setGeometry(option.rect)
            self._updateWidget(combo, index, revisions)

    def currentIndexChanged(self, index):
        self.current_selection = index

    @pyqtSlot(object)
    def cellClicked(self, index):
        isRelevant, revisions = self._isRelevantColumn(index)
        if isRelevant:
            if self.cellEditMode:
                # Remove old persistent model index
                self.parent().closePersistentEditor(self.currentCellIndex)
            # Current model index is stored and added to stay persistent until
            # editing mode is done
            self.currentCellIndex = index
            # If no editor exists, the delegate will create a new editor which
            # means that here ``createEditor`` is called
            self.parent().openPersistentEditor(self.currentCellIndex)
            self.cellEditMode = True
        else:
            if self.cellEditMode:
                self.cellEditMode = False
                # Persistent model index and data namely QComboBox cleaned up
                self.parent().closePersistentEditor(self.currentCellIndex)
