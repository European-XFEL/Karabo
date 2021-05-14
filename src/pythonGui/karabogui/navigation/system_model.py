#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import json
from weakref import WeakValueDictionary

from qtpy.QtCore import QAbstractItemModel, QMimeData, QModelIndex, Qt

import karabogui.access as krb_access
from karabo.common.api import ProxyStatus
from karabogui import icons
from karabogui.alarms.api import get_alarm_icon
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.indicators import get_state_icon_for_status
from karabogui.singletons.api import get_topology

from .context import _UpdateContext
from .utils import get_language_icon


class SystemTreeModel(QAbstractItemModel):

    def __init__(self, parent=None):
        super(SystemTreeModel, self).__init__(parent)

        self._model_index_refs = WeakValueDictionary()

        # Our hierarchy tree
        self.tree = get_topology().system_tree
        self.tree.update_context = _UpdateContext(item_model=self)
        # Add listeners for ``alarm_update`` change event
        self.tree.on_trait_change(self._alarm_update, 'alarm_update')
        self.tree.on_trait_change(self._status_update, 'status_update')

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary
        event_map = {
            KaraboEvent.AccessLevelChanged: self._event_access_level,
            KaraboEvent.StartMonitoringDevice: self._event_monitor,
            KaraboEvent.StopMonitoringDevice: self._event_monitor,
        }
        register_for_broadcasts(event_map)

    def supportedDragActions(self):
        return Qt.CopyAction

    def _event_access_level(self, data):
        self._clear_tree_cache()

    def _event_monitor(self, data):
        node_id = data['device_id']
        self._update_device_info(node_id)

    def index_ref(self, model_index):
        """Get the system node object for a ``QModelIndex``. This is
        essentially equivalent to a weakref and might return None.

        NOTE: We're doing a rather complicated dance here with `internalId` to
        avoid PyQt's behaviour of casting pointers into Python objects, because
        those objects might now be invalid.
        """
        key = model_index.internalId()
        return self._model_index_refs.get(key)

    def createIndex(self, row, column, node):
        """Prophylaxis for QModelIndex.internalPointer...

        We need a nice way to get back to our node objects from a QModelIndex.
        So, we store node instances in model indices, but indirectly. This is
        because QModelIndex is not a strong reference AND these objects will
        tend to outlive the node objects which they reference. So the solution
        is to use a WeakValueDictionary as indirection between Qt and our model
        layer.

        Awesome. QAbstractItemModel can go DIAF.
        """
        key = id(node)
        if key not in self._model_index_refs:
            self._model_index_refs[key] = node
        return super(SystemTreeModel, self).createIndex(row, column, key)

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
        """Reimplemented function of QAbstractItemModel. Counts number of
        Children for a given node in the tree view.
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
        return 3

    def indexInfo(self, index):
        if not index.isValid():
            return {}

        node = self.index_ref(index)
        if node is None:
            return {}

        return node.info()

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
            if hierarchyLevel == 3:
                if node.status is ProxyStatus.ERROR:
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
                return get_state_icon_for_status(node.status)
            elif hierarchyLevel == 1:
                return get_language_icon(node)
        elif column == 2 and role == Qt.DecorationRole:
            if hierarchyLevel == 3:
                alarm_type = node.alarm_info.alarm_type
                if alarm_type:
                    return get_alarm_icon(alarm_type)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        node = self.index_ref(index)
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
            n = self.index_ref(idx)
            if n is None:
                continue
            data.append(n.info())

        mimeData = QMimeData()
        mimeData.setData('treeItems', bytearray(json.dumps(data),
                                                encoding='UTF-8'))
        return mimeData

    def _alarm_update(self, node_ids):
        """ Whenever the ``alarm_update`` event of a ``SystemTree`` is changed
        the view needs to be updated

        :param node_ids: system topology deviceId's to be updated
        """
        assert isinstance(node_ids, set)

        for node_id in node_ids:
            node = self.tree.get_instance_node(node_id)
            if node is not None:
                index = self.createIndex(node.row(), 0, node)
                index = index.siblingAtColumn(2)
                self.dataChanged.emit(index, index, [Qt.DecorationRole])

    def _status_update(self, node_ids):
        """Triggered from the status_update Event from the system tree"""
        assert isinstance(node_ids, set)

        for node_id in node_ids:
            node = self.tree.get_instance_node(node_id)
            if node is not None:
                index = self.createIndex(node.row(), 0, node)
                self.dataChanged.emit(index, index, [Qt.DecorationRole])
                index = index.siblingAtColumn(1)
                self.dataChanged.emit(index, index, [Qt.DecorationRole])

    def _update_device_info(self, node_id, column=0):
        """This function is used to launch a dataChanged signal for a specific
           device Id
        """
        node = self.tree.get_instance_node(node_id)
        if node is not None:
            index = self.createIndex(node.row(), column, node)
            self.dataChanged.emit(index, index, [Qt.DecorationRole])

    def _clear_tree_cache(self):
        """Clear the tree and reset the model to account visibility
        """
        self.beginResetModel()
        try:
            access = krb_access.GLOBAL_ACCESS_LEVEL

            def visitor(node):
                node.is_visible = not (node.visibility > access)

            self.tree.visit(visitor)
            self._model_index_refs.clear()
        finally:
            self.endResetModel()
