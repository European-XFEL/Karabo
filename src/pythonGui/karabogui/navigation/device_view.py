#############################################################################
# Author: <dennis.goeeries@xfel.eu>
# Created on February 12, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSlot, QModelIndex, Qt
from PyQt4.QtGui import (
    QAbstractItemView, QAction, QCursor, QDialog, QHeaderView, QMenu,
    QTreeView)

from karabogui import icons
from karabogui.enums import NavigationItemTypes
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.dialogs.dialogs import ConfigurationFromPastDialog
from karabogui.singletons.api import (
    get_manager, get_network, get_selection_tracker)
from karabogui.widgets.popup import PopupWidget

from .device_model import DeviceTreeModel
from .tools import DeviceSceneHandler


class DeviceTreeView(QTreeView):
    def __init__(self, parent):
        super(DeviceTreeView, self).__init__(parent)
        self._selected_proxy = None  # A BaseDeviceProxy

        model = DeviceTreeModel(parent=self)
        self.setModel(model)
        self.setSelectionModel(model.selectionModel)
        model.rowsInserted.connect(self._items_added)
        model.signalItemChanged.connect(self.onSelectionChanged)

        header = self.header()
        header.setResizeMode(QHeaderView.ResizeToContents)
        # Prevent drag reorder of the header
        header.setMovable(False)

        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        self.handler_list = [DeviceSceneHandler()]
        self.expanded = True
        self.popupWidget = None
        self.header().sectionDoubleClicked.connect(self.onDoubleClickHeader)

        # Setup the context menu
        self._setup_context_menu()
        self.customContextMenuRequested.connect(
            self.onCustomContextMenuRequested)

    def _setup_context_menu(self):
        """Setup the context menu for the device topology"""
        self.setContextMenuPolicy(Qt.CustomContextMenu)

        self.menu = QMenu(self)
        text = "About"
        self.ac_about = QAction("About", self)
        self.ac_about.setStatusTip(text)
        self.ac_about.setToolTip(text)
        self.ac_about.triggered.connect(self.onAbout)

        text = "Get Configuration"
        self.ac_config_past = QAction("Get Configuration", self)
        self.ac_config_past.setStatusTip(text)
        self.ac_config_past.setToolTip(text)
        self.ac_config_past.triggered.connect(self.onGetConfigurationFromPast)

        text = "Shutdown device"
        self.ac_kill_device = QAction(icons.delete, text, self)
        self.ac_kill_device.setStatusTip(text)
        self.ac_kill_device.setToolTip(text)
        self.ac_kill_device.triggered.connect(self.onKillInstance)

        self.menu.addAction(self.ac_about)
        self.menu.addAction(self.ac_config_past)
        self.menu.addAction(self.ac_kill_device)

    def indexInfo(self, index=None):
        """Return the info about the index.

        Defaults to the current index if index is None.
        """
        if index is None:
            index = self.currentIndex()
        return self.model().indexInfo(index)

    def currentIndex(self):
        return self.model().currentIndex()

    def clear(self):
        self._selected_proxy = None
        self.clearSelection()
        self.model().clear()

    def scrollTo(self, index, hint=QAbstractItemView.EnsureVisible):
        """Reimplementation of the Qt function
        """
        self.setExpanded(index, True)
        super(DeviceTreeView, self).scrollTo(index)

    # ----------------------------
    # Events

    def mouseDoubleClickEvent(self, event):
        index = self.currentIndex()
        node = self.model().index_ref(index)
        # QModelIndexes tend to outlive the node objects which they reference!
        if node is None:
            return
        info = node.info()
        for handler in self.handler_list:
            if handler.can_handle(info):
                handler.handle(info)
                event.accept()
                return

        super(DeviceTreeView, self).mouseDoubleClickEvent(event)

    # ----------------------------
    # Slots

    @pyqtSlot(QModelIndex, int, int)
    def _items_added(self, parent_index, start, end):
        """React to the addition of an item (or items).
        """
        # Bail immediately if not the first item
        if start != 0:
            return

        self.expand(parent_index)

    @pyqtSlot(str, object)
    def onSelectionChanged(self, item_type, proxy):
        """Called by the data model when an item is selected
        """
        self._selected_proxy = proxy

        # Grab control of the global selection
        get_selection_tracker().grab_selection(self.model().selectionModel)

        # Tell the configurator
        if item_type not in ('class', 'device'):
            # servers and hosts clear the configurator
            proxy = None
        broadcast_event(KaraboEventSender.ShowConfiguration, {'proxy': proxy})

    @pyqtSlot()
    def onDoubleClickHeader(self):
        if self.expanded:
            self.collapseAll()
        else:
            self.expandAll()
        self.expanded = not self.expanded

    @pyqtSlot()
    def onAbout(self):
        index = self.currentIndex()
        node = self.model().index_ref(index)
        if node is None:
            return
        if self.popupWidget is None:
            self.popupWidget = PopupWidget(parent=self)
        self.popupWidget.setInfo(node.attributes)

        pos = QCursor.pos()
        pos.setX(pos.x() + 10)
        pos.setY(pos.y() + 10)
        self.popupWidget.move(pos)
        self.popupWidget.show()

    @pyqtSlot()
    def onGetConfigurationFromPast(self):
        info = self.indexInfo()
        dialog = ConfigurationFromPastDialog(parent=self)
        dialog.move(QCursor.pos())
        if dialog.exec_() == QDialog.Accepted:
            device_id = info.get('deviceId')
            # Karabo time points are in UTC
            time_point = dialog.ui_timepoint.dateTime().toUTC()
            # Explicitly specifiy ISODate!
            time = str(time_point.toString(Qt.ISODate))
            get_network().onGetConfigurationFromPast(device_id, time=time)

    @pyqtSlot(object)
    def onCustomContextMenuRequested(self, pos):
        info = self.indexInfo()
        node_type = info.get('type', NavigationItemTypes.UNDEFINED)
        if node_type is NavigationItemTypes.DEVICE:
            self.menu.exec_(QCursor.pos())

    @pyqtSlot()
    def onKillInstance(self):
        info = self.indexInfo()
        node_type = info.get('type')
        manager = get_manager()

        if node_type is NavigationItemTypes.DEVICE:
            deviceId = info.get('deviceId')
            manager.shutdownDevice(deviceId)
