#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 22, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSignal, Qt
from PyQt4.QtGui import (
    QItemSelection, QItemSelectionModel, QSortFilterProxyModel)

from karabogui import globals as krb_globals
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.singletons.api import get_topology

CLASS_LEVEL = 2
DEVICE_LEVEL = 3


class TopologyFilterModel(QSortFilterProxyModel):
    signalItemChanged = pyqtSignal(str, object)  # type, BaseDeviceProxy

    def __init__(self, source_model=None, parent=None):
        super(TopologyFilterModel, self).__init__(parent)
        self.setSourceModel(source_model)
        self.setFilterKeyColumn(0)
        self.setFilterRole(Qt.DisplayRole)
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)

        event_map = {
            KaraboEvent.ShowDevice: self._event_show_device
        }
        register_for_broadcasts(event_map)

    def _event_show_device(self, data):
        self.selectNodeById(data['deviceId'])

    def filterAcceptsRow(self, source_row, source_parent):
        if self.filterRegExp().isEmpty():
            pass
        # XXX: More filters later
        model = self.sourceModel()
        source_index = model.index(source_row, self.filterKeyColumn(),
                                   source_parent)
        node = model.index_ref(source_index)
        if node.level in [CLASS_LEVEL, DEVICE_LEVEL] and not node.is_visible:
            return False
        return True

    def currentIndex(self):
        """Retrieve the current index for context menu actions
        """
        index = self.selectionModel.currentIndex()
        source_index = self.mapToSource(index)

        return source_index

    def selectNode(self, node):
        """Select the given `node` of type `SystemTreeNode` if not None,
        otherwise nothing is selected

        :param node: The `SystemTreeNode` which should be selected
        """
        if node is not None:
            source_index = self.sourceModel().createIndex(node.row(), 0, node)
            index = self.mapFromSource(source_index)
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

    def selectIndex(self, index):
        """Select the given `index` of type `QModelIndex` if this is not None

        :param index: Index of the filter model
        """
        if index is None:
            self.selectionModel.selectionChanged.emit(QItemSelection(),
                                                      QItemSelection())
            return

        self.selectionModel.setCurrentIndex(index,
                                            QItemSelectionModel.ClearAndSelect)

        treeview = super(TopologyFilterModel, self).parent()
        treeview.scrollTo(index)

    def index_ref(self, source_index):
        """Index refs requires a source index
        """
        return self.sourceModel().index_ref(source_index)

    def indexInfo(self, index):
        if not index.isValid():
            return {}
        node = self.index_ref(index)
        if node is None:
            return {}
        return node.info()

    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        if not selectedIndexes:
            return

        node = None
        index = selectedIndexes[0]
        source_index = self.mapToSource(index)

        if not source_index.isValid():
            level = 0
        else:
            node = self.index_ref(source_index)
            if node is None:
                return
            level = node.level

        if level == 0:
            proxy = None
            item_type = 'other'
        elif level == 1:
            proxy = None
            item_type = 'server'
        if level == 2:
            classId = node.node_id
            serverId = node.parent.node_id
            proxy = get_topology().get_class(serverId, classId)
            item_type = 'class'
        elif level == 3:
            deviceId = node.node_id
            proxy = get_topology().get_device(deviceId)
            item_type = 'device'

        self.signalItemChanged.emit(item_type, proxy)
