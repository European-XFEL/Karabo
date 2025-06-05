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
from qtpy import uic
from qtpy.QtCore import QAbstractItemModel, QModelIndex, Qt, Slot
from qtpy.QtGui import QColor
from qtpy.QtWidgets import QAbstractItemView, QDialog, QHeaderView
from traits.api import Any, Bool, HasStrictTraits, Instance, Int, List, String

from karabogui.indicators import PROPERTY_READONLY_COLOR
from karabogui.singletons.api import get_config

from .utils import get_dialog_ui

HEADER_LABELS = ["Name", "Setting"]
NAME_COLUMN = 0
SETTING_COLUMN = 1


class ConfigNode(HasStrictTraits):
    # The name of our configuration item
    name = String
    # The parent node
    parent_node = Instance("ConfigNode", allow_none=True)
    # The list of children
    children = List(Instance("ConfigNode"))
    # Check whether this config node is configurable or not
    editable = Bool
    # Level whether this config node belongs to a group or child
    level = Int(0)
    # data type of the property, e.g. bool
    dtype = Any

    def get_value(self):
        return get_config()[self.name]

    def set_value(self, value):
        get_config()[self.name] = value

    def appendChild(self, item):
        self.children.append(item)

    def child(self, row):
        return self.children[row]

    def row(self):
        if self.parent_node is None:
            return 0
        return self.parent_node.children.index(self)

    def parent(self):
        return self.parent_node


class ConfigurationModel(QAbstractItemModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.root = ConfigNode()
        self._get_model_data()

    def columnCount(self, parent=None):
        """Reimplemented function of QAbstractItemModel
        """
        return len(HEADER_LABELS)

    def headerData(self, column, orientation, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel
        """
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return HEADER_LABELS[column]

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel
        """
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parent_node = self.root
        else:
            parent_node = parent.internalPointer()

        child_node = parent_node.child(row)
        if child_node is not None:
            return self.createIndex(row, column, child_node)

    def parent(self, index):
        """Reimplemented function of QAbstractItemModel
        """
        if not index.isValid():
            return QModelIndex()

        child_node = index.internalPointer()
        if not child_node:
            return QModelIndex()

        parent_node = child_node.parent()

        if parent_node == self.root:
            return QModelIndex()

        return self.createIndex(parent_node.row(), 0, parent_node)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel"""
        if not index.isValid():
            return Qt.NoItemFlags
        if index.column() != 1:
            return Qt.ItemIsEnabled

        node = index.internalPointer()
        if node is None:
            return Qt.NoItemFlags

        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if node.dtype == bool and node.editable:
            flags |= Qt.ItemIsUserCheckable
        elif node.editable:
            flags |= Qt.ItemIsEditable
        return flags

    def rowCount(self, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel
        """
        if parent.column() > 0:
            return 0
        if not parent.isValid():
            parent = self.root
        else:
            parent = parent.internalPointer()
        return len(parent.children)

    def _get_model_data(self):
        """Setup the model data from the configuration singleton
        """
        config = get_config()
        groups = config.groups()
        for group_name in groups.keys():
            group_node = ConfigNode(name=group_name,
                                    parent_node=self.root)
            self.root.appendChild(group_node)
            for item in groups[group_name]:
                attr_item = ConfigNode(parent_node=group_node,
                                       **item.toDict(), level=1)
                group_node.appendChild(attr_item)

    def setData(self, index, value, role=Qt.EditRole):
        """Reimplemented function of QAbstractItemModel

        Active roles are CheckStateRole, DisplayRole and EditRole
        """
        if value is None:
            return False

        # Get the index's stored object
        node = index.internalPointer()
        if node is None:
            return False

        if role == Qt.CheckStateRole and node.dtype == bool:
            value = value == Qt.Checked
            node.set_value(value)
            self.dataChanged.emit(index, index)
            return True

        elif role in (Qt.DisplayRole, Qt.EditRole):
            node.set_value(value)
            self.dataChanged.emit(index, index)
            return True

        return False

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel"""
        if not index.isValid():
            return None

        node = index.internalPointer()
        column = index.column()
        if column == NAME_COLUMN:
            if role == Qt.DisplayRole:
                return node.name
        elif column == SETTING_COLUMN:
            if node.level == 1:
                data = node.get_value()
                if role == Qt.CheckStateRole:
                    if node.dtype == bool:
                        return Qt.Checked if data else Qt.Unchecked
                elif role == Qt.DisplayRole:
                    return str(data)
                elif role == Qt.ForegroundRole and not node.editable:
                    return QColor(*PROPERTY_READONLY_COLOR)


class ApplicationConfigurationDialog(QDialog):
    """The dialog to view the configuration singleton
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        # Don't block the event loop!
        self.setModal(False)
        filepath = get_dialog_ui("configuration.ui")
        uic.loadUi(filepath, self)

        self.tree_view.setSelectionBehavior(QAbstractItemView.SelectRows)
        item_model = ConfigurationModel(parent=self)
        self.tree_view.setModel(item_model)
        self.tree_view.setAlternatingRowColors(True)
        self.configure_header()
        self.expanded = False

    def configure_header(self):
        """Configure the treeview header to automatically resize its contents
        """
        header = self.tree_view.header()
        header.setSectionResizeMode(0, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QHeaderView.ResizeToContents)
        header.sectionDoubleClicked.connect(self.onDoubleClickHeader)

    @Slot()
    def onDoubleClickHeader(self):
        if self.expanded:
            self.collapseAll()
        else:
            self.expandAll()

    def collapseAll(self):
        self.expanded = False
        self.tree_view.collapseAll()

    def expandAll(self):
        self.expanded = True
        self.tree_view.expandAll()
