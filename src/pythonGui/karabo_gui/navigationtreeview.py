#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 31, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents the treeview of the navigation
   panel containing the items for the host, device server instance and device
   class/instance.
"""
from functools import partial

from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import (QAbstractItemView, QAction, QCursor, QDialog, QMenu,
                         QTreeView)

from karabo.common.api import Capabilities
from karabo_gui import icons
from karabo_gui.dialogs.device_scenes import DeviceScenesDialog
from karabo_gui.enums import NavigationItemTypes
from karabo_gui.popupwidget import PopupWidget
from karabo_gui.request import call_device_slot
from karabo_gui.singletons.api import (get_manager, get_selection_tracker,
                                       get_navigation_model)
from karabo_gui.util import (
    loadConfigurationFromFile, saveConfigurationToFile,
    handle_scene_from_server, set_treeview_header
)


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
        self.expanded = True
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

        text = "Open Device Scene..."
        self.acOpenScene = QAction(text, self)
        self.acOpenScene.setStatusTip(text)
        self.acOpenScene.setToolTip(text)
        self.acOpenScene.triggered.connect(self.onOpenDeviceScene)
        self.mDeviceItem.addAction(self.acOpenScene)

        self.mDeviceItem.addSeparator()
        self.mDeviceItem.addAction(self.acAbout)

    def currentIndex(self):
        return self.model().currentIndex()

    def indexInfo(self, index=None):
        """Return the info about the index.

        Defaults to the current index if index is None.
        """
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

    @pyqtSlot()
    def onAbout(self):
        index = self.currentIndex()
        node = self.model().index_ref(index)
        if node is None:
            return
        popupWidget = PopupWidget(self)
        popupWidget.setInfo(node.attributes)

        pos = QCursor.pos()
        pos.setX(pos.x() + 10)
        pos.setY(pos.y() + 10)
        popupWidget.move(pos)
        popupWidget.show()

    @pyqtSlot()
    def onKillInstance(self):
        info = self.indexInfo()
        node_type = info.get('type')
        manager = get_manager()

        if node_type is NavigationItemTypes.DEVICE:
            deviceId = info.get('deviceId')
            manager.shutdownDevice(deviceId)
        elif node_type is NavigationItemTypes.SERVER:
            serverId = info.get('serverId')
            manager.shutdownServer(serverId)

    @pyqtSlot()
    def onOpenDeviceScene(self):
        info = self.indexInfo()
        dialog = DeviceScenesDialog(device_id=info.get('deviceId', ''))
        if dialog.exec() == QDialog.Accepted:
            device_id = dialog.device_id
            scene_name = dialog.scene_name
            handler = partial(handle_scene_from_server, device_id, scene_name,
                              None)
            call_device_slot(handler, device_id, 'requestScene',
                             name=scene_name)

    @pyqtSlot(object)
    def onCustomContextMenuRequested(self, pos):
        def _test_mask(mask, bit):
            return (mask & bit) == bit

        info = self.indexInfo()
        node_type = info.get('type', NavigationItemTypes.UNDEFINED)
        if node_type is NavigationItemTypes.SERVER:
            self.acAbout.setVisible(True)
            self.mServerItem.exec_(QCursor.pos())
        elif node_type is NavigationItemTypes.CLASS:
            self.acKillDevice.setVisible(False)
            self.acAbout.setVisible(False)
            self.mDeviceItem.exec_(QCursor.pos())
        elif node_type is NavigationItemTypes.DEVICE:
            self.acKillDevice.setVisible(True)
            self.acAbout.setVisible(True)
            has_scenes = _test_mask(info.get('capabilities', 0),
                                    Capabilities.PROVIDES_SCENES)
            self.acOpenScene.setVisible(has_scenes)
            self.mDeviceItem.exec_(QCursor.pos())

    @pyqtSlot(str, object)
    def onSelectionChanged(self, item_type, configuration):
        """Called by the data model when an item is selected
        """
        self._current_configuration = configuration

        # Grab control of the global selection
        get_selection_tracker().grab_selection(self.model().selectionModel)

    @pyqtSlot()
    def onOpenFromFile(self):
        if self._current_configuration is not None:
            loadConfigurationFromFile(self._current_configuration)

    @pyqtSlot()
    def onSaveToFile(self):
        if self._current_configuration is not None:
            saveConfigurationToFile(self._current_configuration)

    @pyqtSlot()
    def onDoubleClickHeader(self):
        if self.expanded:
            self.collapseAll()
        else:
            self.expandAll()
        self.expanded = not self.expanded
