#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
""" This module contains a class which represents a model to display a
hierarchical navigation in a treeview."""

from contextlib import contextmanager
import json

from PyQt4.QtCore import (QAbstractItemModel, QMimeData, QModelIndex,
                          Qt, pyqtSignal)
from PyQt4.QtGui import QItemSelection, QItemSelectionModel
from traits.api import HasStrictTraits, WeakRef

from karabo_gui.alarms.api import get_alarm_icon
from karabo_gui.events import KaraboEventSender, register_for_broadcasts
import karabo_gui.globals as krb_globals
import karabo_gui.icons as icons
from karabo_gui.indicators import get_state_icon_for_status
from karabo_gui.singletons.api import get_topology


class _UpdateContext(HasStrictTraits):
    """A context manager that can be handed off to code which doesn't need to
    know that it's dealing with a Qt QAbstractItemModel.
    """
    item_model = WeakRef(QAbstractItemModel)

    @contextmanager
    def reset_context(self):
        """Provide a context whenever the system tree was cleared then the
        Qt model needs to be reseted.

        NOTE: This method is a context manager wraps the insertion with calls
        to ``QAbstractItemModel.beginResetModel`` and
        ``QAbstractItemModel.endResetModel`` (See Qt documentation)
        """
        try:
            self.item_model.beginResetModel()
            yield
        finally:
            self.item_model.endResetModel()

    @contextmanager
    def insertion_context(self, parent_node, first, last):
        """Provide a context for the addition of multiple children under a
        single parent item.

        NOTE: This method is a context manager wraps the insertion with calls
        to ``QAbstractItemModel.beginInsertRows`` and
        ``QAbstractItemModel.endInsertRows`` (See Qt documentation)
        """
        parent_index = self.item_model.createIndex(parent_node.row(), 0,
                                                   parent_node)

        def gen():
            try:
                self.item_model.beginInsertRows(parent_index, first, last)
                yield
            finally:
                self.item_model.endInsertRows()

        if parent_index.isValid():
            yield from gen()
        else:
            yield

    @contextmanager
    def removal_context(self, tree_node):
        """Provide a context for the removal of a single item from the model.

        NOTE: This method is a context manager which wraps the removal of an
        item with ``QAbstractItemModel.beginRemoveRows`` and
        ``QAbstractItemModel.endRemoveRows`` (See Qt documentation)
        """
        node_row = tree_node.row()
        index = self.item_model.createIndex(node_row, 0, tree_node)
        if index.isValid():
            parent_index = index.parent()
        else:
            parent_index = QModelIndex()

        def gen():
            try:
                self.item_model.beginRemoveRows(parent_index, node_row,
                                                node_row)
                yield
            finally:
                self.item_model.endRemoveRows()

        if parent_index.isValid():
            yield from gen()
        else:
            yield


class NavigationTreeModel(QAbstractItemModel):
    signalItemChanged = pyqtSignal(str, object)  # type, configuration

    def __init__(self, parent=None):
        super(NavigationTreeModel, self).__init__(parent)

        # Our hierarchy tree
        self.tree = get_topology().system_tree
        self.tree.update_context = _UpdateContext(item_model=self)
        # Add listeners for ``needs_update`` change event
        self.tree.on_trait_change(self._needs_update, 'needs_update')

        self.setSupportedDragActions(Qt.CopyAction)
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.StartMonitoringDevice:
            self._toggleMonitoring(data.get('device_id', ''), True)
        elif sender is KaraboEventSender.StopMonitoringDevice:
            self._toggleMonitoring(data.get('device_id', ''), False)
        elif sender is KaraboEventSender.ShowDevice:
            self.selectNode(data.get('deviceId'))
        elif sender is KaraboEventSender.AccessLevelChanged:
            self._needs_update()
        return False

    def clear(self):
        self.tree.clear_all()

    def currentIndex(self):
        return self.selectionModel.currentIndex()

    def selectIndex(self, index):
        if index is None:
            self.selectionModel.selectionChanged.emit(QItemSelection(),
                                                      QItemSelection())
            return

        self.selectionModel.setCurrentIndex(index,
                                            QItemSelectionModel.ClearAndSelect)

    def selectNode(self, node_id):
        """Select the `QModelIndex` with the given `node_id`
        """
        nodes = self.tree.find(node_id)
        if nodes:
            # Select first entry
            node = nodes[0]
            index = self.createIndex(node.row(), 0, node)
            self.selectIndex(index)

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parent_node = self.tree.root
        else:
            parent_node = parent.internalPointer()

        child_node = parent_node.children[row]
        if child_node is not None:
            # Consider visibility
            if child_node.visibility > krb_globals.GLOBAL_ACCESS_LEVEL:
                return QModelIndex()

            return self.createIndex(row, column, child_node)
        else:
            return QModelIndex()

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

        # Consider visibility
        if parent_node.visibility > krb_globals.GLOBAL_ACCESS_LEVEL:
            return QModelIndex()

        return self.createIndex(parent_node.row(), 0, parent_node)

    def rowCount(self, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if parent.column() > 0:
            return None

        if not parent.isValid():
            parent_node = self.tree.root
        else:
            parent_node = parent.internalPointer()

        return len(parent_node.children)

    def columnCount(self, parentIndex=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        return 3

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel.
        """
        column = index.column()
        node = index.internalPointer()
        hierarchyLevel = node.level()

        if column == 0 and role == Qt.DisplayRole:
            return node.node_id
        elif column == 0 and role == Qt.DecorationRole:
            if hierarchyLevel == 0:
                return icons.host
            elif hierarchyLevel == 1:
                return icons.yes
            elif hierarchyLevel == 2:
                return icons.deviceClass
            elif hierarchyLevel == 3:
                if node.status == "error":
                    return icons.deviceInstanceError
                if node.monitoring:
                    return icons.deviceMonitored
                else:
                    return icons.deviceInstance
        elif column == 1 and role == Qt.DecorationRole:
            if hierarchyLevel == 3:
                return get_state_icon_for_status(node.status)
        elif column == 2 and role == Qt.DecorationRole:
            if hierarchyLevel == 3:
                alarm_type = node.alarm_info.alarm_type
                if alarm_type is not None:
                    return get_alarm_icon(alarm_type)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        ret = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if index.internalPointer().level() > 0:
            ret |= Qt.ItemIsDragEnabled
        return ret

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal and section == 0:
                return "Hierarchical view"

    def indexInfo(self, index):
        if not index.isValid():
            return {}
        return index.internalPointer().info()

    def mimeData(self, indices):
        """Reimplemented function of QAbstractItemModel.

        Provide data for Drag & Drop operations.
        """
        # Get one selection per row
        nodes = {idx.row(): idx.internalPointer() for idx in indices
                 if idx.isValid()}
        # Extract info() dictionaries from SystemTreeNode instances
        data = [n.info() for n in nodes.values()]

        mimeData = QMimeData()
        mimeData.setData('treeItems', json.dumps(data))
        return mimeData

    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()

        if not selectedIndexes:
            return

        index = selectedIndexes[0]

        node = None
        if not index.isValid():
            level = 0
        else:
            node = index.internalPointer()
            level = node.level()

        if level == 0:
            conf = None
            item_type = "other"
        elif level == 1:
            conf = None
            item_type = "server"
        if level == 2:
            classId = node.node_id
            serverId = node.parent.node_id
            conf = get_topology().get_class(serverId, classId)
            item_type = conf.type
        elif level == 3:
            deviceId = node.node_id
            conf = get_topology().get_device(deviceId)
            item_type = conf.type

        self.signalItemChanged.emit(item_type, conf)

    def _toggleMonitoring(self, device_id, monitoring):
        nodes = self.tree.find(device_id)
        # There should better be only one instance with this ID
        assert len(nodes) == 1
        node = nodes[0]
        assert node.monitoring != monitoring
        node.monitoring = monitoring
        self._needs_update()

    def _needs_update(self):
        """ Whenever the ``needs_update`` event of a ``SystemTree`` is changed
        the view needs to be updated
        """
        self.layoutChanged.emit()
