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
from PyQt4.QtGui import (QAbstractItemView, QAction, QCursor, QHeaderView,
                         QMenu, QTreeView)

import karabo_gui.icons as icons
from karabo_gui.enums import NavigationItemTypes
from karabo_gui.events import (KaraboBroadcastEvent, KaraboEventSender,
                               register_for_broadcasts)
from karabo_gui.singletons.api import get_manager
from karabo_gui.treewidgetitems.popupwidget import PopupWidget


class NavigationTreeView(QTreeView):
    def __init__(self, parent):
        super(NavigationTreeView, self).__init__(parent)

        manager = get_manager()
        self.setModel(manager.systemTopology)
        self.setSelectionModel(self.model().selectionModel)
        self.model().modelReset.connect(self.expandAll)

        self.header().setResizeMode(0, QHeaderView.ResizeToContents)
        self.header().setResizeMode(1, QHeaderView.Fixed)
        self.header().setResizeMode(2, QHeaderView.Fixed)
        self.setColumnWidth(1, 20)
        self.setColumnWidth(2, 20)
        # NOTE: Since QTreeView always displays the expander in column 0 the
        # additional columns are moved to the front
        self.header().moveSection(1, 0)
        self.header().moveSection(2, 0)

        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        self._setupContextMenu()
        self.customContextMenuRequested.connect(
            self.onCustomContextMenuRequested)
        self.setDragEnabled(True)

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.ShowDevice:
                data = event.data
                self.selectItem(data.get('deviceId'))
                return False
            elif event.sender is KaraboEventSender.AlarmDeviceUpdate:
                data = event.data
                self.model().updateAlarmIndicators(
                    data.get('deviceId'), data.get('alarm_type'))
        return super(NavigationTreeView, self).eventFilter(obj, event)

    def _setupContextMenu(self):
        manager = get_manager()

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
        self.acOpenFromFile.triggered.connect(manager.onOpenFromFile)
        self.mDeviceItem.addAction(self.acOpenFromFile)

        text = "Open configuration from project"
        self.acOpenFromProject = QAction(icons.load, text, self)
        self.acOpenFromProject.setStatusTip(text)
        self.acOpenFromProject.setToolTip(text)
        self.acOpenFromProject.triggered.connect(manager.onOpenFromProject)
        self.mDeviceItem.addAction(self.acOpenFromProject)

        self.mDeviceItem.addSeparator()

        text = "Save configuration as (*.xml)"
        self.acSaveToFile = QAction(icons.saveAs, text, self)
        self.acSaveToFile.setStatusTip(text)
        self.acSaveToFile.setToolTip(text)
        self.acSaveToFile.triggered.connect(manager.onSaveToFile)
        self.mDeviceItem.addAction(self.acSaveToFile)

        text = "Save configuration to project"
        self.acSaveToProject = QAction(icons.saveAs, text, self)
        self.acSaveToProject.setStatusTip(text)
        self.acSaveToProject.setToolTip(text)
        self.acSaveToProject.triggered.connect(manager.onSaveToProject)
        self.mDeviceItem.addAction(self.acSaveToProject)

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

        level = self.model().getHierarchyLevel(index)
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

    def findIndex(self, path):
        # Find modelIndex via path
        return self.model().findIndex(path)

    def selectItem(self, path):
        index = self.findIndex(path)
        self.model().selectIndex(index)

    def clear(self):
        self.clearSelection()
        self.model().clear()

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

    def mimeData(self, items):
        return self.model().mimeData(items)
