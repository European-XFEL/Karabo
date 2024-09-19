#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 12, 2019
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
import json

from qtpy.QtCore import QAbstractItemModel, QMimeData, QModelIndex, Qt

from karabo.common.api import InstanceStatus
from karabogui import icons
from karabogui.singletons.api import get_topology

from .context import _UpdateContext


class DeviceTreeModel(QAbstractItemModel):

    def __init__(self, parent=None):
        super().__init__(parent)
        # Our hierarchy tree
        self.tree = get_topology().device_tree
        self.tree.update_context = _UpdateContext(item_model=self)
        self.tree.on_trait_change(self._status_update, 'status_update')

    def supportedDragActions(self):
        return Qt.CopyAction

    def clear(self):
        self.tree.clear_all()

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parent_node = self.tree.root
        else:
            parent_node = parent.internalPointer()
            if parent_node is None:
                return QModelIndex()

        children = parent_node.children
        return self.createIndex(row, column, children[row])

    def parent(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return QModelIndex()

        child_node = index.internalPointer()
        if child_node is None:
            return QModelIndex()

        parent_node = child_node.parent
        if parent_node is None:
            return QModelIndex()

        if parent_node == self.tree.root:
            return QModelIndex()

        return self.createIndex(parent_node.row(), 0, parent_node)

    def rowCount(self, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.

        Counts number of children for a given node in the tree view.
        """
        if parent.column() > 0:
            # parent with column number > 0 don't have children
            return 0

        if not parent.isValid():
            parent_node = self.tree.root
        else:
            parent_node = parent.internalPointer()
            if parent_node is None:
                return 0

        return len(parent_node.children)

    def columnCount(self, parentIndex=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        return 1

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return

        node = index.internalPointer()
        if node is None:
            return

        column = index.column()
        hierarchyLevel = node.level
        if column == 0 and role == Qt.DisplayRole:
            return node.node_id
        elif column == 0 and role == Qt.DecorationRole:
            if hierarchyLevel == 0:
                return icons.folderDomain
            elif hierarchyLevel == 1:
                return icons.folderType
            elif hierarchyLevel == 2:
                if node.status is InstanceStatus.ERROR:
                    return icons.deviceInstanceError
                else:
                    return icons.deviceInstance

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        node = index.internalPointer()
        if node is None:
            return Qt.NoItemFlags

        ret = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if node.level == 2:
            ret |= Qt.ItemNeverHasChildren
            ret |= Qt.ItemIsDragEnabled
        return ret

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal and section == 0:
                return "Domain - Type - Name"

    def mimeData(self, indices):
        """Reimplemented function of QAbstractItemModel.

        Provide data for Drag & Drop operations.
        """
        # Get one selection per row
        rows = {idx.row(): idx for idx in indices if idx.isValid()}
        # Extract info() dictionaries from SystemTreeNode instances
        data = []
        for idx in rows.values():
            n = idx.internalPointer()
            if n is None:
                continue
            data.append(n.info())

        mimeData = QMimeData()
        mimeData.setData('treeItems', bytearray(json.dumps(data),
                                                encoding='UTF-8'))
        return mimeData

    def indexInfo(self, index):
        if not index.isValid():
            return {}

        node = index.internalPointer()
        if node is None:
            return {}

        return node.info()

    def _status_update(self, node_ids):
        """Triggered from the status_update Event from the device tree"""
        for node_id in node_ids:
            node = self.tree.get_instance_node(node_id)
            if node is not None:
                index = self.createIndex(node.row(), 0, node)
                self.dataChanged.emit(index, index, [Qt.DecorationRole])

    def currentIndex(self):
        return self.selectionModel.currentIndex()
