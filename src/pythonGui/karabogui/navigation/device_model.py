#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 12, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from weakref import WeakValueDictionary

from qtpy.QtCore import (
    Signal, Slot, QAbstractItemModel, QModelIndex, Qt, QItemSelection,
    QItemSelectionModel)

from karabo.common.api import ProxyStatus
from karabogui import globals as krb_globals, icons
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.singletons.api import get_topology
from .context import _UpdateContext


class DeviceTreeModel(QAbstractItemModel):
    signalItemChanged = Signal(str, object)  # type, BaseDeviceProxy

    def __init__(self, parent=None):
        super(DeviceTreeModel, self).__init__(parent)

        self._model_index_refs = WeakValueDictionary()

        # Our hierarchy tree
        self.tree = get_topology().device_tree
        self.tree.update_context = _UpdateContext(item_model=self)
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)

        event_map = {
            KaraboEvent.AccessLevelChanged: self._event_access_level,
        }
        register_for_broadcasts(event_map)

    def supportedDragActions(self):
        return Qt.CopyAction

    def _event_access_level(self, data):
        self._clear_tree_cache()

    def index_ref(self, model_index):
        """Get the system node object for a ``QModelIndex``. This is
        essentially equivalent to a weakref and might return None.
        """
        key = model_index.internalId()
        return self._model_index_refs.get(key)

    def createIndex(self, row, column, node):
        """Prophylaxis for QModelIndex.internalPointer...
        """
        key = id(node)
        if key not in self._model_index_refs:
            self._model_index_refs[key] = node
        return super(DeviceTreeModel, self).createIndex(row, column, key)

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
            parent_node = self.index_ref(parent)
            if parent_node is None:
                return QModelIndex()

        children = parent_node.get_visible_children()
        return self.createIndex(row, column, children[row])

    def parent(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return QModelIndex()

        child_node = self.index_ref(index)
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
            parent_node = self.index_ref(parent)
            if parent_node is None:
                return 0

        return len(parent_node.get_visible_children())

    def columnCount(self, parentIndex=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        return 1

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return

        node = self.index_ref(index)
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
                if node.status is ProxyStatus.ERROR:
                    return icons.deviceInstanceError
                else:
                    return icons.deviceInstance

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        node = self.index_ref(index)
        if node is None:
            return Qt.NoItemFlags

        ret = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if node.level == 2:
            ret |= Qt.ItemNeverHasChildren
        return ret

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal and section == 0:
                return "Domain - Type - Name"

    def indexInfo(self, index):
        if not index.isValid():
            return {}

        node = self.index_ref(index)
        if node is None:
            return {}

        return node.info()

    def _clear_tree_cache(self):
        """Clear the tree and reset the model to account visibility
        """
        self.layoutAboutToBeChanged.emit()
        access = krb_globals.GLOBAL_ACCESS_LEVEL

        def visitor(node):
            node.is_visible = not (node.visibility > access)
            node.clear_cache = True

        self.tree.visit(visitor)
        self.layoutChanged.emit()

    def currentIndex(self):
        return self.selectionModel.currentIndex()

    @Slot(QItemSelection, QItemSelection)
    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()

        if not selectedIndexes:
            return

        node = None
        index = selectedIndexes[0]
        if not index.isValid():
            level = 0
        else:
            node = self.index_ref(index)
            if node is None:
                return
            level = node.level

        if level == 0:
            proxy = None
            item_type = 'domain'
        elif level == 1:
            proxy = None
            item_type = 'type'
        if level == 2:
            deviceId = node.node_id
            proxy = get_topology().get_device(deviceId)
            item_type = 'device'

        self.signalItemChanged.emit(item_type, proxy)

    # ------------------------------------------------------------------
    # Search Model related methods

    def selectIndex(self, index):
        """Select the given `index` of type `QModelIndex` if this is not None
        """
        if index is None:
            self.selectionModel.selectionChanged.emit(QItemSelection(),
                                                      QItemSelection())
            return

        self.selectionModel.setCurrentIndex(index,
                                            QItemSelectionModel.ClearAndSelect)

        treeview = super(DeviceTreeModel, self).parent()
        treeview.scrollTo(index)

    def selectNodeById(self, node_id):
        """Select the `DeviceTreeNode` with the given `node_id`.

        :param node_id: A string which we are looking for in the tree
        """
        nodes = self.findNodes(node_id, full_match=True)
        assert len(nodes) <= 1
        if nodes:
            # Select first entry
            self.selectNode(nodes[0])

    def selectNode(self, node):
        """Select the given `node` of type `DeviceTreeNode`

        If the the node or index is None, nothing is selected

        :param node: The `DeviceTreeNode` which should be selected
        """
        if node is not None:
            index = self.createIndex(node.row(), 0, node)
        else:
            # Select nothing
            index = None
        self.selectIndex(index)

    def findNodes(self, node_id, **kwargs):
        if kwargs.get('access_level') is None:
            kwargs['access_level'] = krb_globals.GLOBAL_ACCESS_LEVEL
        return self.tree.find(node_id, **kwargs)
