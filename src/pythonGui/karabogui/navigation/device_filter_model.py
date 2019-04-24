#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on April 18, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt
from PyQt4.QtGui import (
    QItemSelection, QItemSelectionModel, QSortFilterProxyModel)

from karabogui.singletons.api import get_topology


class DeviceFilterModel(QSortFilterProxyModel):
    signalItemChanged = pyqtSignal(str, object)  # type, BaseDeviceProxy

    def __init__(self, source_model=None, parent=None):
        super(DeviceFilterModel, self).__init__(parent)
        self.setSourceModel(source_model)
        self.setFilterKeyColumn(0)
        self.setFilterRole(Qt.DisplayRole)
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)

    def filterAcceptsRow(self, source_row, source_parent):
        model = self.sourceModel()
        source_index = model.index(source_row, self.filterKeyColumn(),
                                   source_parent)
        if source_index.isValid():
            node = model.index_ref(source_index)
            return node.is_visible

        return super(DeviceFilterModel, self).filterAcceptsRow(
            source_row, source_parent)

    # --------------------------------------------------------------------
    # Index Methods

    def index_ref(self, index):
        source_index = self.mapToSource(index)
        return self.sourceModel().index_ref(source_index)

    def currentIndex(self):
        """Retrieve the current index for context menu actions"""
        return self.selectionModel.currentIndex()

    def indexInfo(self, index):
        source_index = self.mapToSource(index)
        return self.sourceModel().indexInfo(source_index)

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

        treeview = super(DeviceFilterModel, self).parent()
        treeview.scrollTo(index)

    @pyqtSlot(QItemSelection, QItemSelection)
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
            node = self.sourceModel().index_ref(source_index)
            if node is None:
                return
            level = node.level

        if level == 0:
            proxy = None
            item_type = 'domain'
        elif level == 1:
            proxy = None
            item_type = 'type'
        elif level == 2:
            deviceId = node.device_id
            proxy = get_topology().get_device(deviceId)
            item_type = 'device'

        self.signalItemChanged.emit(item_type, proxy)
