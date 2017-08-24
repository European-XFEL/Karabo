#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 31, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents the treeview of the navigation
   panel containing the items for the host, device server instance and device
   class/instance.
"""
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAbstractItemView, QAction, QCursor, QMenu, QTreeView

import karabo_gui.icons as icons
from karabo_gui.enums import NavigationItemTypes
from karabo_gui.popupwidget import PopupWidget
from karabo_gui.singletons.api import (get_manager, get_selection_tracker,
                                       get_navigation_model)
from karabo_gui.util import (
    loadConfigurationFromFile, saveConfigurationToFile, set_treeview_header)


class NavigationTreeView(QTreeView):
    def __init__(self, parent):
        super(NavigationTreeView, self).__init__(parent)
        self._current_configuration = None

        model = get_navigation_model()
        self.setModel(model)
        self.setSelectionModel(model.selectionModel)
        model.rowsInserted.connect(self._items_added)
        model.signalItemChanged.connect(self.onSelectionChanged)

        set_treeview_header(self)

        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        self._setupContextMenu()
        self.customContextMenuRequested.connect(
            self.onCustomContextMenuRequested)
        self.setDragEnabled(True)

        # by default all path are expanded
        self.treeExpanded = True
        self.header().sectionDoubleClicked.connect(self.onDoubleClickHeader)

    def _setupContextMenu(self):
        self.setContextMenuPolicy(Qt.CustomContextMenu)

        text = "About"
        self.acAbout = QAction("About", self)
        self.acAbout.setStatusTip(text)
        self.acAbout.setToolTip(text)
        self.acAbout.triggered.connect(self.onAbout)

        # Device server instance menu
        self.mServerItem = QMenu(self)

        text = "Shutdown instance"
        self.acKillServer = QAction(icons.delete, text, self)
        self.acKillServer.setStatusTip(text)
        self.acKillServer.setToolTip(text)
        self.acKillServer.triggered.connect(self.onKillInstance)
        self.mServerItem.addAction(self.acKillServer)

        self.mServerItem.addSeparator()
        self.mServerItem.addAction(self.acAbout)

        # Device class/instance menu
        self.mDeviceItem = QMenu(self)

        text = "Open configuration (*.xml)"
        self.acOpenFromFile = QAction(icons.load, text, self)
        self.acOpenFromFile.setStatusTip(text)
        self.acOpenFromFile.setToolTip(text)
        self.acOpenFromFile.triggered.connect(self.onOpenFromFile)
        self.mDeviceItem.addAction(self.acOpenFromFile)

        self.mDeviceItem.addSeparator()

        text = "Save configuration as (*.xml)"
        self.acSaveToFile = QAction(icons.saveAs, text, self)
        self.acSaveToFile.setStatusTip(text)
        self.acSaveToFile.setToolTip(text)
        self.acSaveToFile.triggered.connect(self.onSaveToFile)
        self.mDeviceItem.addAction(self.acSaveToFile)

        text = "Shutdown instance"
        self.acKillDevice = QAction(icons.delete, text, self)
        self.acKillDevice.setStatusTip(text)
        self.acKillDevice.setToolTip(text)
        self.acKillDevice.triggered.connect(self.onKillInstance)
        self.mDeviceItem.addAction(self.acKillDevice)

        self.mDeviceItem.addSeparator()
        self.mDeviceItem.addAction(self.acAbout)

    def currentIndex(self):
        return self.model().currentIndex()

    def currentIndexType(self):
        """Returns the type of the current index (NODE, DEVICE_SERVER_INSTANCE,
           DEVICE_CLASS, DEVICE_INSTANCE"""

        index = self.currentIndex()
        if not index.isValid():
            return NavigationItemTypes.UNDEFINED

        level = index.internalPointer().level()
        if level == 0:
            return NavigationItemTypes.HOST
        elif level == 1:
            return NavigationItemTypes.SERVER
        elif level == 2:
            return NavigationItemTypes.CLASS
        elif level == 3:
            return NavigationItemTypes.DEVICE

        return NavigationItemTypes.UNDEFINED

    def indexInfo(self, index=None):
        """ return the info about the index.

        Defaults to the current index if index is None."""
        if index is None:
            index = self.currentIndex()
        return self.model().indexInfo(index)

    def clear(self):
        self._current_configuration = None
        self.clearSelection()
        self.model().clear()

    # ----------------------------
    # Slots

    def _items_added(self, parent_index, start, end):
        """React to the addition of an item (or items).
        """
        # Bail immediately if not the first item
        if start != 0:
            return

        self.expand(parent_index)

    def onAbout(self):
        index = self.currentIndex()
        node = index.internalPointer()
        popupWidget = PopupWidget(self)
        popupWidget.setInfo(node.attributes)

        pos = QCursor.pos()
        pos.setX(pos.x() + 10)
        pos.setY(pos.y() + 10)
        popupWidget.move(pos)
        popupWidget.show()

    def onKillInstance(self):
        itemInfo = self.indexInfo()
        type = itemInfo.get('type')
        manager = get_manager()

        if type is NavigationItemTypes.DEVICE:
            deviceId = itemInfo.get('deviceId')
            manager.shutdownDevice(deviceId)
        elif type is NavigationItemTypes.SERVER:
            serverId = itemInfo.get('serverId')
            manager.shutdownServer(serverId)

    def onCustomContextMenuRequested(self, pos):
        type = self.currentIndexType()
        # Show context menu for DEVICE_CLASS and DEVICE_INSTANCE
        if type is NavigationItemTypes.SERVER:
            self.mServerItem.exec_(QCursor.pos())
        elif type is NavigationItemTypes.CLASS:
            self.acKillDevice.setVisible(False)
            self.acAbout.setVisible(False)
            self.mDeviceItem.exec_(QCursor.pos())
        elif type is NavigationItemTypes.DEVICE:
            self.acKillDevice.setVisible(True)
            self.acAbout.setVisible(True)
            self.mDeviceItem.exec_(QCursor.pos())

    def onSelectionChanged(self, item_type, configuration):
        """Called by the data model when an item is selected
        """
        self._current_configuration = configuration

        # Grab control of the global selection
        get_selection_tracker().grab_selection(self.model().selectionModel)

    def onOpenFromFile(self):
        if self._current_configuration is not None:
            loadConfigurationFromFile(self._current_configuration)

    def onSaveToFile(self):
        if self._current_configuration is not None:
            saveConfigurationToFile(self._current_configuration)

    def onDoubleClickHeader(self):
        if self.treeExpanded:
            self.collapseAll()
        else:
            self.expandAll()
        self.treeExpanded = not self.treeExpanded
