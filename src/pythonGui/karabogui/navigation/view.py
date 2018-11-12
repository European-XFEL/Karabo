#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 31, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import (QAbstractItemView, QAction, QCursor, QDialog, QMenu,
                         QTreeView)
from traits.api import Undefined

from karabo.common.api import Capabilities
from karabogui import icons
from karabogui import messagebox
from karabogui.dialogs.device_capability import DeviceCapabilityDialog
from karabogui.enums import NavigationItemTypes
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.request import call_device_slot
from karabogui.singletons.api import (
    get_manager, get_selection_tracker, get_topology)
from karabogui.util import (
    get_scene_from_server, handle_scene_from_server,
    load_configuration_from_file, save_configuration_to_file,
    set_treeview_header)
from karabogui.widgets.popup import PopupWidget
from .model import NavigationTreeModel


class NavigationTreeView(QTreeView):
    def __init__(self, parent):
        super(NavigationTreeView, self).__init__(parent)
        self._selected_proxy = None  # A BaseDeviceProxy

        model = NavigationTreeModel(parent=self)
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

        text = "Shutdown server"
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

        text = "Shutdown device"
        self.acKillDevice = QAction(icons.delete, text, self)
        self.acKillDevice.setStatusTip(text)
        self.acKillDevice.setToolTip(text)
        self.acKillDevice.triggered.connect(self.onKillInstance)
        self.mDeviceItem.addAction(self.acKillDevice)

        text = "Open Device Scene..."
        self.acOpenScene = QAction(text, self)
        self.acOpenScene.setStatusTip(text)
        self.acOpenScene.setToolTip(text)
        self.acOpenScene.setVisible(False)  # Most items don't have scenes
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
        self._selected_proxy = None
        self.clearSelection()
        self.model().clear()

    def scrollTo(self, index, hint=QAbstractItemView.EnsureVisible):
        """Reimplementation of the Qt function
        """
        self.setExpanded(index, True)
        super(NavigationTreeView, self).scrollTo(index)

    # ----------------------------
    # Events

    def mouseDoubleClickEvent(self, event):
        index = self.currentIndex()
        node = self.model().index_ref(index)
        # QModelIndexes tend to outlive the node objects which they reference!
        if node is None:
            return
        info = node.info()
        navigation_type = info.get('type')
        if navigation_type is not NavigationItemTypes.DEVICE:
            return

        device_id = info.get('deviceId')
        capabilities = info.get('capabilities')

        def _test_mask(mask, bit):
            return (mask & bit) == bit

        has_scene = _test_mask(capabilities, Capabilities.PROVIDES_SCENES)
        if not has_scene:
            messagebox.show_warning("The device <b>{}</b> does not provide a "
                                    "scene!".format(device_id), "Warning",
                                    modal=False)
            return

        def _config_handler():
            """Act on the arrival of the configuration
            """
            proxy.on_trait_change(_config_handler, 'config_update',
                                  remove=True)
            scenes = proxy.binding.value.availableScenes.value
            if scenes is Undefined or not len(scenes):
                messagebox.show_warning(
                    "The device <b>{}</b> does not specify a scene "
                    "name!".format(device_id), modal=False)
            else:
                scene_name = scenes[0]
                get_scene_from_server(device_id, scene_name)

        def _schema_handler():
            """Act on the arrival of the schema
            """
            proxy.on_trait_change(_schema_handler, 'schema_update',
                                  remove=True)
            scenes = proxy.binding.value.availableScenes.value
            if scenes is Undefined:
                proxy.on_trait_change(_config_handler, 'config_update')
            elif not len(scenes):
                messagebox.show_warning(
                    "The device <b>{}</b> does not specify a scene "
                    "name!".format(device_id), modal=False)
            else:
                scene_name = scenes[0]
                get_scene_from_server(device_id, scene_name)

        proxy = get_topology().get_device(device_id)
        if not len(proxy.binding.value):
            # We completely miss our schema and wait for it.
            proxy.on_trait_change(_schema_handler, 'schema_update')
        elif proxy.binding.value.availableScenes.value is Undefined:
            # The configuration did not yet arrive and we cannot get
            # a scene name from the availableScenes. We wait for the
            # configuration to arrive and install a handler.
            proxy.on_trait_change(_config_handler, 'config_update')
        else:
            scenes = proxy.binding.value.availableScenes.value
            if not len(scenes):
                # The device might not have a scene name in property
                messagebox.show_warning(
                    "The device <b>{}</b> does not specify a scene "
                    "name!".format(device_id), modal=False)
            else:
                scene_name = scenes[0]
                get_scene_from_server(device_id, scene_name)

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
        popupWidget = PopupWidget(parent=self)
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
        dialog = DeviceCapabilityDialog(
            device_id=info.get('deviceId', ''),
            capability=Capabilities.PROVIDES_SCENES)
        if dialog.exec() == QDialog.Accepted:
            device_id = dialog.device_id
            scene_name = dialog.capa_name
            handler = partial(handle_scene_from_server, device_id, scene_name,
                              None, None)
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
    def onOpenFromFile(self):
        if self._selected_proxy is not None:
            load_configuration_from_file(self._selected_proxy)

    @pyqtSlot()
    def onSaveToFile(self):
        if self._selected_proxy is not None:
            save_configuration_to_file(self._selected_proxy)

    @pyqtSlot()
    def onDoubleClickHeader(self):
        if self.expanded:
            self.collapseAll()
        else:
            self.expandAll()
        self.expanded = not self.expanded
