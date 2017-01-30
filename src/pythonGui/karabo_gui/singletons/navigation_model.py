#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
""" This module contains a class which represents a model to display a
hierarchical navigation in a treeview."""

import json

from PyQt4.QtCore import (QAbstractItemModel, QMimeData, QModelIndex,
                          Qt, pyqtSignal)
from PyQt4.QtGui import QItemSelection, QItemSelectionModel
from traits.api import HasStrictTraits, String, WeakRef

from karabo.common.states import State
from karabo_gui.events import (KaraboBroadcastEvent, KaraboEventSender,
                               register_for_broadcasts)
import karabo_gui.globals as krb_globals
import karabo_gui.icons as icons
from karabo_gui.indicators import ALARM_ICONS, get_state_icon, NONE
from karabo_gui.singletons.api import get_topology


class _UpdateContext(HasStrictTraits):
    """A context manager that can be handed off to code which doesn't need to
    know that it's dealing with a Qt QAbstractItemModel.
    """
    item_model = WeakRef(QAbstractItemModel)
    last_selection = String

    def __enter__(self):
        self.last_selection = self.item_model.currentSelectionPath()
        self.item_model.beginResetModel()
        return self

    def __exit__(self, *exc):
        self.item_model.endResetModel()

        if self.last_selection != '':
            self.item_model.selectPath(self.last_selection)
        self.last_selection = ''

        return False


class NavigationTreeModel(QAbstractItemModel):
    signalItemChanged = pyqtSignal(str, object)  # type, configuration

    def __init__(self, parent=None):
        super(NavigationTreeModel, self).__init__(parent)

        # Our hierarchy tree
        self.tree = get_topology().system_tree
        self.tree.update_context = _UpdateContext(item_model=self)

        self.setSupportedDragActions(Qt.CopyAction)
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            sender = event.sender
            data = event.data
            if sender is KaraboEventSender.StartMonitoringDevice:
                self._toggleMonitoring(data.get('device_id', ''), True)
            elif sender is KaraboEventSender.StopMonitoringDevice:
                self._toggleMonitoring(data.get('device_id', ''), False)
            elif event.sender is KaraboEventSender.ShowDevice:
                self.selectPath(data.get('deviceId'))
            elif event.sender is KaraboEventSender.AlarmDeviceUpdate:
                device_id = data.get('deviceId')
                alarm_type = data.get('alarm_type')
                self._updateAlarmIndicators(device_id, alarm_type)
            return False
        return super(NavigationTreeModel, self).eventFilter(obj, event)

    def currentSelectionPath(self):
        """Returns the current selection path, or '' if nothing is selected.
        """
        # Get last selection path
        selectedIndexes = self.selectionModel.selectedIndexes()
        if selectedIndexes:
            return selectedIndexes[0].internalPointer().path
        else:
            return ''

    def has(self, path):
        return self.tree.find(path) is not None

    def eraseDevice(self, instanceId):
        index = self.findIndex(instanceId)
        if index is None or not index.isValid():
            return

        next_selection = ''
        if self.selectionModel.isSelected(index):
            next_selection = index.internalPointer().parent.path

        self.tree.remove_device(instanceId)

        if next_selection:
            self.selectPath(next_selection)

    def eraseServer(self, instanceId):
        return self.tree.remove_server(instanceId)

    def detectExistingInstances(self, config):
        """This function checks whether instances already exist in the tree.

        \Returns a list with all existing instanceIds and a list with existing
        serverClassIds.
        """
        return self.tree.clear_existing(config)

    def globalAccessLevelChanged(self):
        lastSelectionPath = self.currentSelectionPath()
        self.modelReset.emit()
        if lastSelectionPath != '':
            self.selectPath(lastSelectionPath)

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

    def findIndex(self, path):
        node = self.tree.find(path)
        if node is not None:
            return self.createIndex(node.row(), 0, node)
        return None

    def selectPath(self, path):
        index = self.findIndex(path)
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
        if not child_node:
            return QModelIndex()

        parent_node = child_node.parent
        if not parent_node:
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
            return node.display_name
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
                state = State.ERROR if node.status == 'error' else State.ACTIVE
                # XXX: Maybe show more color options in the future
                return get_state_icon(state)
        elif column == 2 and role == Qt.DecorationRole:
            if hierarchyLevel == 3:
                if node.alarm_type is not None and node.alarm_type != NONE:
                    return ALARM_ICONS[node.alarm_type].icon

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
            classId = node.display_name
            serverId = node.parent.display_name
            conf = get_topology().get_class(serverId, classId)
            item_type = conf.type
        elif level == 3:
            deviceId = node.display_name
            conf = get_topology().get_device(deviceId)
            item_type = conf.type

        self.signalItemChanged.emit(item_type, conf)

    def _toggleMonitoring(self, device_id, monitoring):
        index = self.findIndex(device_id)
        if index is not None and index.isValid():
            assert index.internalPointer().monitoring != monitoring
            index.internalPointer().monitoring = monitoring
            self.dataChanged.emit(index, index)

    def _updateAlarmIndicators(self, device_id, alarm_type):
        index = self.findIndex(device_id)
        if index is not None and index.isValid():
            node = index.internalPointer()
            node.alarm_type = alarm_type
            self.dataChanged.emit(index, index)
