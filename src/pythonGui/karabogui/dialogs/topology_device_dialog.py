#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on September 22, 2022
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
from qtpy import uic
from qtpy.QtCore import (
    QItemSelection, QSortFilterProxyModel, QStringListModel, Qt, Slot)
from qtpy.QtWidgets import QAbstractItemView, QDialog, QDialogButtonBox

from karabo.common.api import Interfaces
from karabogui.topology.api import getTopology

from .utils import get_dialog_ui


class InstanceFilterModel(QSortFilterProxyModel):

    def __init__(self, topology, parent=None):
        super().__init__(parent)
        self.topology = topology
        self._interface = None
        self.setFilterCaseSensitivity(Qt.CaseInsensitive)
        self.setFilterRole(Qt.DisplayRole)
        self.setFilterKeyColumn(0)

    def setInterface(self, text):
        """Set a interface bit for the filtering"""
        if text == "All Devices":
            self._interface = None
        else:
            self._interface = Interfaces[text]

    def filterAcceptsRow(self, source_row, source_parent):
        model = self.sourceModel()
        source_index = model.index(source_row, self.filterKeyColumn(),
                                   source_parent)
        if source_index.isValid():
            if self._interface is not None:
                deviceId = source_index.data()
                attrs = self.topology[deviceId, ...]
                interfaces = attrs.get("interfaces", 0)
                if not self._check_interface(interfaces, self._interface):
                    return False

        return super().filterAcceptsRow(source_row, source_parent)

    def _check_interface(self, mask, bit):
        return (mask & bit) == bit


class TopologyDeviceDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("topology_device_dialog.ui"), self)
        self.setModal(False)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.device_id = ""

        for interface in Interfaces:
            self.ui_interface.addItem(interface.name)

        topology = getTopology()["device"]
        self.filter_model = InstanceFilterModel(topology, parent=self)
        self.filter_model.setSourceModel(QStringListModel(topology))
        self.filter_model.setFilterFixedString("")

        self.ui_devices_view.setWordWrap(True)
        self.ui_devices_view.setAlternatingRowColors(True)
        self.ui_devices_view.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.ui_devices_view.setModel(self.filter_model)
        self.ui_filter.textChanged.connect(self.onFilterChanged)
        self.ui_clear.clicked.connect(self.clearFilter)
        self.ui_interface.currentIndexChanged.connect(self.onFilterChanged)

        selection = self.ui_devices_view.selectionModel()
        selection.selectionChanged.connect(self.onSelectionChanged)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

    @Slot()
    def onFilterChanged(self):
        self.filter_model.setInterface(self.ui_interface.currentText())
        self.filter_model.setFilterFixedString(self.ui_filter.text())

    @Slot()
    def clearFilter(self):
        self.ui_interface.setCurrentIndex(0)
        self.ui_filter.setText("")
        self.onFilterChanged()

    @Slot(QItemSelection, QItemSelection)
    def onSelectionChanged(self, selected, unselected):
        enabled = len(selected) > 0
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    def done(self, result):
        device_id = ""

        selection = self.ui_devices_view.selectionModel()
        index = selection.currentIndex()
        if index.isValid():
            index = self.filter_model.mapToSource(index)
            device_id = index.data()

        self.device_id = device_id
        super().done(result)
