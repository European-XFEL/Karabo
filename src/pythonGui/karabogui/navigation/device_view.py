#############################################################################
# Author: <dennis.goeeries@xfel.eu>
# Created on February 12, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtCore import pyqtSlot, Qt, QPoint
from PyQt5.QtGui import QCursor
from PyQt5.QtWidgets import (
    QAbstractItemView, QAction, QDialog, QHeaderView, QMenu, QTreeView)

from karabogui import icons
from karabogui import messagebox
from karabogui.enums import AccessRole, NavigationItemTypes
from karabogui.globals import access_role_allowed
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.dialogs.dialogs import ConfigurationFromPastDialog
from karabogui.singletons.api import (
    get_manager, get_network, get_selection_tracker)
from karabogui.util import open_documentation_link
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
        model.signalItemChanged.connect(self.onSelectionChanged)
        model.modelReset.connect(self.resetExpand)
        header = self.header()
        header.setResizeMode(QHeaderView.ResizeToContents)
        # Prevent drag reorder of the header
        header.setSectionsMovable(False)

        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        self.handler_list = [DeviceSceneHandler()]
        self.expanded = False
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
        self.ac_about = QAction(icons.about, "About", self)
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

        text = "Documentation"
        self.ac_docu = QAction(icons.weblink, text, self)
        self.ac_docu.triggered.connect(self.onGetDocumenation)

        self.menu.addAction(self.ac_config_past)
        self.menu.addAction(self.ac_kill_device)
        self.menu.addSeparator()
        self.menu.addAction(self.ac_about)
        self.menu.addAction(self.ac_docu)

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
        # XXX: We used to expand the index here!
        super(DeviceTreeView, self).scrollTo(index, hint)

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
        broadcast_event(KaraboEvent.ShowConfiguration, {'proxy': proxy})

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
        archive = info['attributes'].get('archive', False)
        if not archive:
            # Display a hint for the operator that currently the device is not
            # archived/logged if so. Do not see a parent here to block!
            messagebox.show_warning(
                f"The device {info.get('deviceId')} is currently NOT "
                f"archived! If it was not archived at the requested point in "
                f"time but before that, you will receive an outdated "
                f"configuration.")

        dialog = ConfigurationFromPastDialog(parent=self)
        dialog.move(QCursor.pos())
        if dialog.exec_() == QDialog.Accepted:
            device_id = info.get('deviceId')
            # Karabo time points are in UTC
            time_point = dialog.ui_timepoint.dateTime().toUTC()
            # Explicitly specifiy ISODate!
            time = str(time_point.toString(Qt.ISODate))
            get_network().onGetConfigurationFromPast(device_id, time=time)

    @pyqtSlot(QPoint)
    def onCustomContextMenuRequested(self, pos):
        info = self.indexInfo()
        node_type = info.get('type', NavigationItemTypes.UNDEFINED)
        if node_type is NavigationItemTypes.DEVICE:
            # Killing services is access level dependent!
            enable = access_role_allowed(AccessRole.SERVICE_EDIT)
            self.ac_kill_device.setEnabled(enable)
            self.menu.exec_(QCursor.pos())

    @pyqtSlot()
    def onKillInstance(self):
        info = self.indexInfo()
        node_type = info.get('type')
        manager = get_manager()

        if node_type is NavigationItemTypes.DEVICE:
            deviceId = info.get('deviceId')
            manager.shutdownDevice(deviceId, parent=self)

    @pyqtSlot()
    def onGetDocumenation(self):
        deviceId = self.indexInfo().get('deviceId')
        open_documentation_link(deviceId)

    @pyqtSlot()
    def onDoubleClickHeader(self):
        if self.expanded:
            self.collapseAll()
        else:
            self.expandAll()

    @pyqtSlot()
    def resetExpand(self):
        self.expanded = False

    def collapseAll(self):
        self.expanded = False
        super(DeviceTreeView, self).collapseAll()

    def expandAll(self):
        self.expanded = True
        super(DeviceTreeView, self).expandAll()
