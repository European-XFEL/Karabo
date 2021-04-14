#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 22, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import (
    Signal, Slot, Qt, QItemSelection, QItemSelectionModel,
    QSortFilterProxyModel)

from karabogui import globals as krb_globals
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.singletons.api import get_topology

HOST_LEVEL = 0
SERVER_LEVEL = 1
CLASS_LEVEL = 2
DEVICE_LEVEL = 3


class TopologyFilterModel(QSortFilterProxyModel):
    signalItemChanged = Signal(str, object)  # type, BaseDeviceProxy

    def __init__(self, source_model=None, parent=None):
        super(TopologyFilterModel, self).__init__(parent)
        self.setSourceModel(source_model)
        self.setFilterCaseSensitivity(Qt.CaseInsensitive)
        self.setFilterRole(Qt.DisplayRole)
        self.setRecursiveFilteringEnabled(True)
        self.setFilterKeyColumn(0)
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
        # Normally, this should not happen, we go safe here!
        # We expect a QModelIndex to have a node behind. But these nodes
        # are stored weak and might vanish. The original Qt C++ code
        # returns `True` for invalid QModelIndex (`None` object on Pointer).
        node = source_index.internalPointer()
        if node is None:
            return True
        if not node.is_visible:
            return False

        return super(TopologyFilterModel, self).filterAcceptsRow(
            source_row, source_parent)

    # --------------------------------------------------------------------
    # Index Methods

    def index_ref(self, index):
        source_index = self.mapToSource(index)
        if not source_index.isValid():
            return None
        return self.sourceModel().index_ref(source_index)

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

        treeview = super(TopologyFilterModel, self).parent()
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
        if kwargs.get('access_level') is None:
            kwargs['access_level'] = krb_globals.GLOBAL_ACCESS_LEVEL
        return self.sourceModel().tree.find(node_id, **kwargs)
