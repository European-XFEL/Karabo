#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 28, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QAbstractItemView, QStandardItemModel, QTreeView,
                         QStandardItem)

from karabo.middlelayer import Hash, VectorHash
from karabo_gui.schema import Schema
from karabo_gui.widget import EditableWidget


HEADER_LABELS = ['source', 'type', 'behavior', 'monitor', 'access']


class EditableListOfNodes(EditableWidget):
    category = VectorHash
    priority = 10
    alias = "List Of Nodes"

    def __init__(self, box, parent):
        super(EditableListOfNodes, self).__init__(box)
        self._internal_widget = QTreeView(parent=parent)
        self._internal_widget.setSelectionBehavior(
            QAbstractItemView.SelectRows)
        model = QStandardItemModel(parent=self._internal_widget)
        model.setHorizontalHeaderLabels(HEADER_LABELS)
        self._internal_widget.setModel(model)

    @classmethod
    def isCompatible(cls, box, readonly):
        return not readonly

    @property
    def editWidget(self):
        return self._internal_widget

    @property
    def widget(self):
        return self._internal_widget

    @property
    def value(self):
        # return self._internal_widget.model.data()
        return []  # TODO: Return edited VectorHash

    def valueChanged(self, box, value, timestamp=None):
        if value is None:  # or isinstance(value, Dummy):
            return

        self._update(value)

    def _update(self, value):
        """Update the model to the incoming `value`
        """
        def _recurse(parent_key, parent_value, parent_item, checkable=False):
            child_item = QStandardItem(parent_key)
            child_item.setCheckable(checkable)
            if checkable:
                # TODO: Get the actual state
                child_item.setCheckState(Qt.Checked)
            parent_item.appendRow(child_item)
            if isinstance(parent_value, Schema):
                for r_key, r_value, _ in parent_value.dict.items():
                    new_key = "{}.{}".format(parent_key, r_key)
                    _recurse(new_key, r_value, parent_value)
            elif isinstance(parent_value, Hash):
                row = child_item.rowCount()
                column = 0
                for k, v in parent_value.items():
                    add_item = QStandardItem(str(v))
                    child_item.setChild(row, column, add_item)
                    column += 1

        # Clear model before updating
        self._internal_widget.model().clear()
        tree_model = self._internal_widget.model()
        tree_model.setHorizontalHeaderLabels(HEADER_LABELS)
        root_item = tree_model.invisibleRootItem()
        for entry in value:
            for k, v in entry.items():
                _recurse(k, v, root_item, checkable=True)
