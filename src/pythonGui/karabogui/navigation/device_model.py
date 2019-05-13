#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 12, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from weakref import WeakValueDictionary

from PyQt4.QtCore import QAbstractItemModel, QModelIndex, Qt

from karabo.common.api import DeviceStatus
from karabogui import globals as krb_globals, icons
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.singletons.api import get_topology
from .context import _UpdateContext


class DeviceTreeModel(QAbstractItemModel):

    def __init__(self, parent=None):
        super(DeviceTreeModel, self).__init__(parent)

        self._model_index_refs = WeakValueDictionary()

        # Our hierarchy tree
        self.tree = get_topology().device_tree
        self.tree.update_context = _UpdateContext(item_model=self)
        # Add listeners for ``needs_update`` change event
        self.tree.on_trait_change(self._needs_update, 'needs_update')

        self.setSupportedDragActions(Qt.CopyAction)
        event_map = {
            KaraboEvent.AccessLevelChanged: self._event_access_level,
        }
        register_for_broadcasts(event_map)

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

        children = parent_node.children
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
                if node.status is DeviceStatus.ERROR:
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
        return ret

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal and section == 0:
                return "Domain - Type - Member"

    def indexInfo(self, index):
        if not index.isValid():
            return {}

        node = self.index_ref(index)
        if node is None:
            return {}

        return node.info()

    def _needs_update(self):
        """ Whenever the ``needs_update`` event of a ``SystemTree`` is changed
        the view needs to be updated
        """
        self.layoutAboutToBeChanged.emit()
        self.layoutChanged.emit()

    def _clear_tree_cache(self):
        """Clear the tree and reset the model to account visibility
        """
        self.beginResetModel()
        access = krb_globals.GLOBAL_ACCESS_LEVEL

        def visitor(node):
            node.is_visible = not (node.visibility > access)

        self.tree.visit(visitor)
        self.endResetModel()
