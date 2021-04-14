#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 22, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import (
    Signal, Slot, Qt, QItemSelection, QItemSelectionModel,
    QSortFilterProxyModel)

from karabo.common.enums import Interfaces
from karabogui.singletons.api import get_topology


class DeviceFilterModel(QSortFilterProxyModel):
    signalItemChanged = Signal(str, object)  # type, BaseDeviceProxy

    def __init__(self, source_model=None, parent=None):
        super(DeviceFilterModel, self).__init__(parent)
        self.setSourceModel(source_model)
        self.setFilterCaseSensitivity(Qt.CaseInsensitive)
        self.setFilterRole(Qt.DisplayRole)
        self.setRecursiveFilteringEnabled(True)
        self.setFilterKeyColumn(0)
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)
        self._interface = None

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

        if self._interface is None:
            return super(DeviceFilterModel, self).filterAcceptsRow(
                source_row, source_parent)
        else:
            interface = self._check_interface(node.interfaces, self._interface)
            if self.filterRegExp().isEmpty():
                return interface

            match = super(DeviceFilterModel, self).filterAcceptsRow(
                source_row, source_parent)
            return interface and match

    def _check_interface(self, mask, bit):
        return (mask & bit) == bit

    def setInterface(self, text):
        """Set a interface bit for the filtering"""
        if text == "All Devices":
            self._interface = None
        else:
            self._interface = Interfaces[text]

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

        treeview = super(DeviceFilterModel, self).parent()
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
