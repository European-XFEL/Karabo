#############################################################################
# Author: <dennis.goeeries@xfel.eu>
# Created on February 12, 2019
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

from qtpy.QtCore import QPoint, Qt, Slot
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import (
    QAbstractItemView, QAction, QDialog, QHeaderView, QMenu, QTreeView)

from karabo.common.api import Capabilities
from karabogui import icons
from karabogui.access import AccessRole, access_role_allowed
from karabogui.const import TOOLTIP_INIT_CONFIG, TOOLTIP_RUNTIME_CONFIG
from karabogui.dialogs.api import (
    ConfigurationFromPastDialog, DeviceCapabilityDialog,
    InitConfigurationDialog)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.itemtypes import NavigationItemTypes
from karabogui.request import get_scene_from_server
from karabogui.singletons.api import get_manager, get_selection_tracker
from karabogui.widgets.popup import PopupWidget

from .device_filter_model import DeviceFilterModel
from .device_model import DeviceTreeModel
from .tools import DeviceSceneHandler


class DeviceTreeView(QTreeView):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("deviceTreeView")
        self._selected_proxy = None  # A BaseDeviceProxy

        model = DeviceTreeModel(parent=self)
        proxy_model = DeviceFilterModel(parent=self,
                                        source_model=model)
        self.setModel(proxy_model)
        self.setSelectionModel(proxy_model.selectionModel)
        proxy_model.signalItemChanged.connect(self.onSelectionChanged)
        proxy_model.modelReset.connect(self.resetExpand)
        header = self.header()
        header.setSectionResizeMode(QHeaderView.ResizeToContents)
        # Prevent drag reorder of the header
        header.setSectionsMovable(False)

        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        self.handler_list = [DeviceSceneHandler()]
        self.expanded = False
        self.popupWidget = None
        self.header().sectionDoubleClicked.connect(self.onDoubleClickHeader)
        self.setDragEnabled(True)

        # Setup the context menu
        self._setup_context_menu()
        self.customContextMenuRequested.connect(
            self.onCustomContextMenuRequested)
        self.setUniformRowHeights(True)

    def _setup_context_menu(self):
        """Setup the context menu for the device topology"""
        self.setContextMenuPolicy(Qt.CustomContextMenu)

        self.menu = QMenu(self)
        text = "About"
        self.ac_about = QAction(icons.about, "About", self)
        self.ac_about.setStatusTip(text)
        self.ac_about.setToolTip(text)
        self.ac_about.triggered.connect(self.onAbout)

        text = "Runtime configuration"
        self.ac_config_past = QAction(icons.clock, text, self)
        self.ac_config_past.setStatusTip(text)
        self.ac_config_past.setToolTip(TOOLTIP_RUNTIME_CONFIG)
        self.ac_config_past.triggered.connect(self.onGetConfigurationFromPast)

        text = "Init configuration"
        self.ac_config_name = QAction(icons.initconfig, text, self)
        self.ac_config_name.setStatusTip(text)
        self.ac_config_name.setToolTip(TOOLTIP_INIT_CONFIG)
        self.ac_config_name.triggered.connect(self.onGetInitConfiguration)

        text = "Open device scene..."
        self.ac_open_device_scene = QAction(text, self)
        self.ac_open_device_scene.setStatusTip(text)
        self.ac_open_device_scene.setToolTip(text)
        self.ac_open_device_scene.setVisible(False)
        self.ac_open_device_scene.triggered.connect(self.onOpenDeviceScene)

        text = "Shutdown device"
        self.ac_kill_device = QAction(icons.delete, text, self)
        self.ac_kill_device.setStatusTip(text)
        self.ac_kill_device.setToolTip(text)
        self.ac_kill_device.triggered.connect(self.onKillInstance)

        self.menu.addAction(self.ac_config_past)
        self.menu.addAction(self.ac_config_name)
        self.menu.addSeparator()
        self.menu.addAction(self.ac_kill_device)
        self.menu.addAction(self.ac_open_device_scene)
        self.menu.addSeparator()
        self.menu.addAction(self.ac_about)

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
        super().scrollTo(index, hint)

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

        super().mouseDoubleClickEvent(event)

    # ----------------------------
    # Slots

    @Slot(str, object)
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

    @Slot()
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

    @Slot()
    def onOpenDeviceScene(self):
        info = self.indexInfo()
        dialog = DeviceCapabilityDialog(
            device_id=info.get('deviceId', ''),
            capability=Capabilities.PROVIDES_SCENES,
            parent=self)
        if dialog.exec() == QDialog.Accepted:
            device_id = dialog.device_id
            scene_name = dialog.capa_name
            get_scene_from_server(device_id, scene_name=scene_name)

    @Slot()
    def onGetConfigurationFromPast(self):
        info = self.indexInfo()
        device_id = info['deviceId']
        dialog = ConfigurationFromPastDialog(instance_id=device_id,
                                             parent=self)
        dialog.move(QCursor.pos())
        dialog.show()
        dialog.raise_()
        dialog.activateWindow()

    @Slot()
    def onGetInitConfiguration(self):
        info = self.indexInfo()
        device_id = info.get('deviceId')
        dialog = InitConfigurationDialog(instance_id=device_id,
                                         parent=self)
        dialog.move(QCursor.pos())
        dialog.show()
        dialog.raise_()
        dialog.activateWindow()

    @Slot(QPoint)
    def onCustomContextMenuRequested(self, pos):
        def _test_mask(mask, bit):
            return (mask & bit) == bit

        info = self.indexInfo()
        node_type = info.get("type", NavigationItemTypes.UNDEFINED)
        if node_type is NavigationItemTypes.DEVICE:
            # Killing services is access level dependent!
            enable = access_role_allowed(AccessRole.SERVICE_EDIT)
            self.ac_kill_device.setEnabled(enable)
            has_scenes = _test_mask(info.get("capabilities", 0),
                                    Capabilities.PROVIDES_SCENES)
            self.ac_open_device_scene.setVisible(has_scenes)
            self.menu.exec(QCursor.pos())

    @Slot()
    def onKillInstance(self):
        info = self.indexInfo()
        node_type = info.get('type')
        manager = get_manager()

        if node_type is NavigationItemTypes.DEVICE:
            deviceId = info.get('deviceId')
            manager.shutdownDevice(deviceId, parent=self)

    @Slot()
    def onDoubleClickHeader(self):
        if self.expanded:
            self.collapseAll()
        else:
            self.expandAll()

        index = self.model().currentIndex()
        if index.isValid():
            self.scrollTo(index)

    @Slot()
    def resetExpand(self):
        self.expanded = False

    def collapseAll(self):
        self.expanded = False
        super().collapseAll()

    def expandAll(self):
        self.expanded = True
        super().expandAll()
