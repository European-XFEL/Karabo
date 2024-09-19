#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
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
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.indicators import get_instance_info_icon
from karabogui.singletons.api import get_topology

from .context import _UpdateContext
from .utils import get_language_icon


class SystemTreeModel(QAbstractItemModel):

    def __init__(self, parent=None):
        super().__init__(parent)
        # Our hierarchy tree
        self.tree = get_topology().system_tree
        self.tree.update_context = _UpdateContext(item_model=self)
        self.tree.on_trait_change(self._status_update, 'status_update')

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary
        event_map = {
            KaraboEvent.StartMonitoringDevice: self._event_monitor,
            KaraboEvent.StopMonitoringDevice: self._event_monitor,
        }
        register_for_broadcasts(event_map)

    def supportedDragActions(self):
        return Qt.CopyAction

    def _event_monitor(self, data):
        node_id = data['device_id']
        self._update_device_info(node_id)

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
        """Reimplemented function of QAbstractItemModel. Counts number of
        Children for a given node in the tree view.
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
        return 2

    def indexInfo(self, index):
        if not index.isValid():
            return {}

        node = index.internalPointer()
        if node is None:
            return {}

        return node.info()

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
            if hierarchyLevel == 3:
                if node.status is InstanceStatus.ERROR:
                    return icons.deviceInstanceError
                if node.monitoring:
                    return icons.deviceMonitored
                else:
                    return icons.deviceInstance
            elif hierarchyLevel == 2:
                return icons.deviceClass
            elif hierarchyLevel == 1:
                return icons.yes
            elif hierarchyLevel == 0:
                return icons.host
        elif column == 1 and role == Qt.DecorationRole:
            if hierarchyLevel == 3:
                return get_instance_info_icon(node.status)
            elif hierarchyLevel == 1:
                return get_language_icon(node)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        node = index.internalPointer()
        if node is None:
            return Qt.NoItemFlags

        ret = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if node.level == 3:
            # Devices can be dragged into other widgets
            ret |= Qt.ItemIsDragEnabled
            # Devices never have children optimization
            ret |= Qt.ItemNeverHasChildren
        return ret

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal and section == 0:
                return "Host - Server - Class - Device"

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

    def _status_update(self, node_ids):
        """Triggered from the status_update Event from the system tree"""
        assert isinstance(node_ids, set)

        for node_id in node_ids:
            node = self.tree.get_instance_node(node_id)
            if node is not None:
                index = self.createIndex(node.row(), 0, node)
                self.dataChanged.emit(index, index, [Qt.DecorationRole])
                index = index.sibling(node.row(), 1)
                self.dataChanged.emit(index, index, [Qt.DecorationRole])

    def _update_device_info(self, node_id, column=0):
        """This function is used to launch a dataChanged signal for a specific
           device Id
        """
        node = self.tree.get_instance_node(node_id)
        if node is not None:
            index = self.createIndex(node.row(), column, node)
            self.dataChanged.emit(index, index, [Qt.DecorationRole])
