#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 28, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, QModelIndex, Qt
from PyQt4.QtGui import (QAbstractItemView, QStandardItemModel, QTreeView,
                         QStandardItem)

from karabo.middlelayer import Hash
from karabo_gui.schema import ListOfNodes
from karabo_gui.widget import EditableWidget

HEADER_LABELS = ['source', 'type', 'behavior', 'monitored', 'access']
NODE_CLASS_NAME = '_RunConfiguratorGroup'


class RunConfiguratorEdit(EditableWidget):
    category = ListOfNodes
    priority = 10
    alias = "List Of Nodes"

    # XXX: This widget ONLY supports a LIST_ELEMENT from the RunConfigurator
    displayType = 'RunConfigurator'

    def __init__(self, box, parent):
        super(RunConfiguratorEdit, self).__init__(box)
        self.widget = QTreeView(parent=parent)
        self.widget.setSelectionBehavior(QAbstractItemView.SelectRows)
        model = QStandardItemModel(parent=self.widget)
        model.setHorizontalHeaderLabels(HEADER_LABELS)
        model.itemChanged.connect(self._item_edited)
        self.widget.setModel(model)

    @classmethod
    def isCompatible(cls, box, readonly):
        return not readonly

    @property
    def value(self):
        def _build_source_hash(item, row):
            hsh = Hash()
            for col, name in enumerate(HEADER_LABELS):
                hsh[name] = item.child(row, col).text()
            return hsh

        values = []
        tree_model = self.widget.model()
        root_item = tree_model.invisibleRootItem()
        for i in range(tree_model.rowCount(root_item.index())):
            item = tree_model.item(i)
            item_hash = Hash('groupId', item.text(),
                             'use', item.checkState() == Qt.Checked)
            item_hash['sources'] = [_build_source_hash(item, j)
                                    for j in range(item.rowCount())]
            values.append(Hash(NODE_CLASS_NAME, item_hash))
        return values

    def valueChanged(self, box, value, timestamp=None):
        if value is not None:  # or isinstance(value, Dummy):
            self._update(value)

    @pyqtSlot(object)
    def _item_edited(self, item):
        # Only react when top-level items are edited (ie: check-state changes)
        if item.parent() is None:
            self.onEditingFinished(self.value)

    def _update(self, value):
        """Update the model to the incoming `value`
        """
        def _build(group_obj, parent_item):
            group_item = QStandardItem(group_obj.get('groupId', 'NONAME'))
            group_item.setCheckable(True)
            if bool(group_obj.get('use', False)):
                group_item.setCheckState(Qt.Checked)
            parent_item.appendRow(group_item)
            row, column = 0, 0
            for src in group_obj.get('sources', []):
                for attr in HEADER_LABELS:
                    add_item = QStandardItem(str(src.get(attr, '')))
                    add_item.setEditable(False)
                    group_item.setChild(row, column, add_item)
                    column += 1
                row, column = row + 1, 0

        # Clear model before updating
        self.widget.model().clear()
        tree_model = self.widget.model()
        tree_model.setHorizontalHeaderLabels(HEADER_LABELS)
        root_item = tree_model.invisibleRootItem()
        for entry in value:
            _build(entry[NODE_CLASS_NAME], root_item)
