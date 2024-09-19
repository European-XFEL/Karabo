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

from karabo.common.enums import Interfaces
from karabogui.singletons.api import get_topology


class DeviceFilterModel(QSortFilterProxyModel):
    signalItemChanged = Signal(str, object)  # type, BaseDeviceProxy

    def __init__(self, source_model=None, parent=None):
        super().__init__(parent)
        self._interface = None
        self.setFilterCaseSensitivity(Qt.CaseInsensitive)
        self.setFilterRole(Qt.DisplayRole)
        self.setFilterKeyColumn(0)
        self.setRecursiveFilteringEnabled(True)
        self.setDynamicSortFilter(False)
        self.setSourceModel(source_model)
        self.selectionModel = QItemSelectionModel(self, self)
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)

    def filterAcceptsRow(self, source_row, source_parent):
        model = self.sourceModel()
        source_index = model.index(source_row, self.filterKeyColumn(),
                                   source_parent)
        if source_index.isValid():
            node = source_index.internalPointer()
            if node is None:
                return True
            if self._interface is not None:
                interface = self._check_interface(node.interfaces,
                                                  self._interface)
                if not interface:
                    return False

        return super().filterAcceptsRow(source_row, source_parent)

    def _check_interface(self, mask, bit):
        return (mask & bit) == bit

    def setFilterSelection(self, text):
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

        if level == 0:
            proxy = None
            item_type = 'domain'
        elif level == 1:
            proxy = None
            item_type = 'type'
        elif level == 2:
            deviceId = node.node_id
            proxy = get_topology().get_device(deviceId)
            item_type = 'device'

        self.signalItemChanged.emit(item_type, proxy)
