#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 22, 2018
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
from qtpy.QtCore import (
    QItemSelection, QItemSelectionModel, QSortFilterProxyModel, Qt, Signal,
    Slot)

from karabo.common.api import InstanceStatus
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.singletons.api import get_topology

HOST_LEVEL = 0
SERVER_LEVEL = 1
CLASS_LEVEL = 2
DEVICE_LEVEL = 3


class TopologyFilterModel(QSortFilterProxyModel):
    signalItemChanged = Signal(str, object)  # type, BaseDeviceProxy

    def __init__(self, source_model=None, parent=None):
        super().__init__(parent)
        self._filter_status = None
        self.setFilterCaseSensitivity(Qt.CaseInsensitive)
        self.setFilterRole(Qt.DisplayRole)
        self.setFilterKeyColumn(0)
        self.setRecursiveFilteringEnabled(True)
        self.setDynamicSortFilter(False)
        self.setSourceModel(source_model)
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)
        event_map = {
            KaraboEvent.ShowDevice: self._event_show_device
        }
        register_for_broadcasts(event_map)

    def _event_show_device(self, data):
        self.selectNodeById(data['deviceId'])

    def filterAcceptsRow(self, source_row, source_parent):
        model = self.sourceModel()
        source_index = model.index(source_row, self.filterKeyColumn(),
                                   source_parent)
        if source_index.isValid():
            node = source_index.internalPointer()
            if node is None:
                return True

            if self._filter_status is not None:
                status = node.status is self._filter_status
                if not status:
                    return False

        return super().filterAcceptsRow(source_row, source_parent)

    def setFilterSelection(self, text):
        """Set a filter status for the filtering"""
        if text == "No Status Filtering":
            self._filter_status = None
        elif text == "Health Status [OK]":
            self._filter_status = InstanceStatus.OK
        elif text == "Health Status [ERROR]":
            self._filter_status = InstanceStatus.ERROR
        elif text == "Health Status [UNKNOWN]":
            self._filter_status = InstanceStatus.UNKNOWN
        else:
            self._filter_status = None

    # --------------------------------------------------------------------
    # Index Methods

    def index_ref(self, index):
        source_index = self.mapToSource(index)
        if not source_index.isValid():
            return None
        return source_index.internalPointer()

    def currentIndex(self):
        """Retrieve the current index for context menu actions"""
        return self.selectionModel.currentIndex()

    def indexInfo(self, index):
        source_index = self.mapToSource(index)
        if not source_index.isValid():
            return {}
        return self.sourceModel().indexInfo(source_index)

    def selectIndex(self, index):
        """Select the given `index` of type `QModelIndex` if this is not None

        :param index: Index of the filter model
        """
        if index is None or not index.isValid():
            self.selectionModel.clearSelection()
            return

        self.selectionModel.setCurrentIndex(index,
                                            QItemSelectionModel.ClearAndSelect)

        treeview = super().parent()
        treeview.scrollTo(index)

    @Slot(QItemSelection, QItemSelection)
    def onSelectionChanged(self, selected, deselected):
        source_selection = self.mapSelectionToSource(selected)
        selectedIndexes = source_selection.indexes()
        if not selectedIndexes:
            return

        index = selectedIndexes[0]
        if index is None or not index.isValid():
            return

        node = index.internalPointer()
        if node is None:
            return
        level = node.level

        if level == HOST_LEVEL:
            proxy = None
            item_type = 'other'
        elif level == SERVER_LEVEL:
            proxy = None
            item_type = 'server'
        elif level == CLASS_LEVEL:
            classId = node.node_id
            serverId = node.parent.node_id
            proxy = get_topology().get_class(serverId, classId)
            item_type = 'class'
        elif level == DEVICE_LEVEL:
            deviceId = node.node_id
            proxy = get_topology().get_device(deviceId)
            item_type = 'device'

        self.signalItemChanged.emit(item_type, proxy)

    # --------------------------------------------------------------------
    # Node Selection Methods

    def selectNode(self, node):
        """Select the given `node` of type `SystemTreeNode` if not None,
        otherwise nothing is selected

        :param node: The `SystemTreeNode` which should be selected
        """
        if node is not None:
            source_index = self.sourceModel().createIndex(node.row(), 0, node)
            index = self.mapFromSource(source_index)
            if not index.isValid():
                index = None
        else:
            # Select nothing
            index = None

        self.selectIndex(index)

    def selectNodeById(self, node_id):
        """Select the `SystemTreeNode` with the given `node_id`.

        :param node_id: A string which we are looking for in the tree
        """
        nodes = self.findNodes(node_id, full_match=True)
        assert len(nodes) <= 1
        if nodes:
            # Select first entry
            self.selectNode(nodes[0])

    def findNodes(self, node_id, **kwargs):
        return self.sourceModel().tree.find(node_id, **kwargs)
