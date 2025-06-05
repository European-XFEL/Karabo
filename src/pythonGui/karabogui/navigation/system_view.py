#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 31, 2013
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
from functools import partial

from qtpy.QtCore import QPoint, Qt, Slot
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import (
    QAbstractItemView, QAction, QDialog, QHeaderView, QInputDialog, QMenu,
    QTreeView)

from karabo.common.api import KARABO_DAEMON_MANAGER, Capabilities
from karabo.native import Timestamp
from karabogui import icons, messagebox
from karabogui.access import (
    AccessRole, access_role_allowed, get_access_level_for_role)
from karabogui.dialogs.api import (
    ConfigurationFromPastDialog, DeviceCapabilityDialog,
    InitConfigurationDialog)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.itemtypes import NavigationItemTypes
from karabogui.navigation.system_filter_model import TopologyFilterModel
from karabogui.request import call_device_slot, get_scene_from_server
from karabogui.singletons.api import (
    get_manager, get_network, get_selection_tracker)
from karabogui.topology.api import is_device_online
from karabogui.util import (
    get_reason_parts, load_configuration_from_file, move_to_cursor,
    save_configuration_to_file)
from karabogui.widgets.popup import PopupWidget

from .system_model import SystemTreeModel
from .tools import DeviceSceneHandler, ServerLogHandler

DISABLED_TOOLTIP = (
    f"Requires minimum '"
    f"{get_access_level_for_role(AccessRole.SERVICE_EDIT)}' access level.")


class SystemTreeView(QTreeView):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("systemTreeView")
        self._selected_proxy = None  # A BaseDeviceProxy

        model = SystemTreeModel(parent=self)
        proxy_model = TopologyFilterModel(parent=self,
                                          source_model=model)
        self.setModel(proxy_model)
        self.setSelectionModel(proxy_model.selectionModel)

        header = self.header()
        header.moveSection(1, 0)
        header.setSectionResizeMode(0, QHeaderView.Stretch)
        header.setSectionResizeMode(1, QHeaderView.Fixed)
        header.setStretchLastSection(False)
        icon_size = 26
        header.setMaximumSectionSize(icon_size)
        header.resizeSection(1, icon_size)

        proxy_model.setFilterKeyColumn(0)
        proxy_model.signalItemChanged.connect(self.onSelectionChanged)
        proxy_model.modelReset.connect(self.resetExpand)

        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        self._setupContextMenu()
        self.customContextMenuRequested.connect(
            self.onCustomContextMenuRequested)
        self.setDragEnabled(True)

        self.handler_list = [DeviceSceneHandler(), ServerLogHandler()]

        self.expanded = False
        self.popupWidget = None
        self.header().sectionDoubleClicked.connect(self.onDoubleClickHeader)
        self.setUniformRowHeights(True)

    def _setupContextMenu(self):
        self.setContextMenuPolicy(Qt.CustomContextMenu)

        # Device server instance menu
        self.mHostItem = QMenu(self)
        text = "Network information"
        self.acNetworkInfo = QAction(icons.about, text, self)
        self.acNetworkInfo.setStatusTip(text)
        self.acNetworkInfo.setToolTip(text)
        self.acNetworkInfo.triggered.connect(self.onNetworkInfo)
        self.mHostItem.addAction(self.acNetworkInfo)

        text = "About"
        self.acAbout = QAction(icons.about, "About", self)
        self.acAbout.setStatusTip(text)
        self.acAbout.setToolTip(text)
        self.acAbout.triggered.connect(self.onAbout)

        text = "Time information"
        self.acTimeInformation = QAction(icons.clock, text, self)
        self.acTimeInformation.triggered.connect(self.onTimeInformation)
        self.acTimeInformation.setVisible(False)  # Classes don't have time

        # Device server instance menu
        self.mServerItem = QMenu(self)
        self.mServerItem.setToolTipsVisible(True)

        text = "Set logger level"
        self.acLoggerLevel = QAction(icons.edit, text, self)
        self.acLoggerLevel.setStatusTip(text)
        self.acLoggerLevel.setToolTip(text)
        self.acLoggerLevel.triggered.connect(self.onLoggerLevel)
        self.mServerItem.addAction(self.acLoggerLevel)

        text = "Shutdown server"
        self.acKillServer = QAction(icons.delete, text, self)
        self.acKillServer.setStatusTip(text)
        self.acKillServer.setToolTip(text)
        self.acKillServer.triggered.connect(self.onKillInstance)
        self.mServerItem.addAction(self.acKillServer)

        self.mServerItem.addSeparator()
        self.mServerItem.addAction(self.acAbout)

        self.mServerItem.addAction(self.acTimeInformation)

        # Device class/instance menu
        self.mDeviceItem = QMenu(self)
        self.mDeviceItem.setToolTipsVisible(True)

        text = "Open configuration (*.xml)"
        self.acOpenFromFile = QAction(icons.load, text, self)
        self.acOpenFromFile.setStatusTip(text)
        self.acOpenFromFile.setToolTip(text)
        self.acOpenFromFile.triggered.connect(self.onOpenFromFile)
        self.mDeviceItem.addAction(self.acOpenFromFile)

        text = "Save configuration as (*.xml)"
        self.acSaveToFile = QAction(icons.saveAs, text, self)
        self.acSaveToFile.setStatusTip(text)
        self.acSaveToFile.setToolTip(text)
        self.acSaveToFile.triggered.connect(self.onSaveToFile)
        self.mDeviceItem.addAction(self.acSaveToFile)

        self.mDeviceItem.addSeparator()

        text = "Get configuration (time)"
        self.ac_config_past = QAction(icons.clock, text, self)
        self.ac_config_past.setStatusTip(text)
        self.ac_config_past.setToolTip(text)
        self.ac_config_past.triggered.connect(self.onGetConfigurationFromPast)
        self.mDeviceItem.addAction(self.ac_config_past)

        text = "Get && save configuration (init)"
        self.ac_config_name = QAction(text, self)
        self.ac_config_name.setStatusTip(text)
        self.ac_config_name.setToolTip(text)
        self.ac_config_name.triggered.connect(self.onGetInitConfiguration)
        self.mDeviceItem.addAction(self.ac_config_name)

        self.mDeviceItem.addSeparator()

        text = "Shutdown device"
        self.acKillDevice = QAction(icons.delete, text, self)
        self.acKillDevice.setStatusTip(text)
        self.acKillDevice.setToolTip(text)
        self.acKillDevice.triggered.connect(self.onKillInstance)
        self.mDeviceItem.addAction(self.acKillDevice)

        text = "Open device scene..."
        self.acOpenScene = QAction(text, self)
        self.acOpenScene.setStatusTip(text)
        self.acOpenScene.setToolTip(text)
        self.acOpenScene.setVisible(False)  # Most items don't have scenes
        self.acOpenScene.triggered.connect(self.onOpenDeviceScene)
        self.mDeviceItem.addAction(self.acOpenScene)

        self.mDeviceItem.addSeparator()

        self.mDeviceItem.addAction(self.acTimeInformation)
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
                handler.handle(info, parent=self)
                event.accept()
                return

        super().mouseDoubleClickEvent(event)

    # ----------------------------
    # Slots

    @Slot()
    def onNetworkInfo(self):
        if not is_device_online(KARABO_DAEMON_MANAGER):
            messagebox.show_error(
                "The KaraboDaemonManager is not online ...", parent=self)
            return

        index = self.currentIndex()
        node = self.model().index_ref(index)
        if node is None:
            return

        def show_network_info(success, reply):
            if not success:
                reason, details = get_reason_parts(reply)
                messagebox.show_error(reason, details=details, parent=self)
                return

            if self.popupWidget is None:
                self.popupWidget = PopupWidget(parent=self)

            payload = reply["payload"]
            network = payload["network"]
            self.popupWidget.setInfo(network)
            self.popupWidget.adjustSize()
            move_to_cursor(self.popupWidget)
            self.popupWidget.show()

        host = node.info()["hostId"]
        call_device_slot(
            show_network_info, KARABO_DAEMON_MANAGER, "requestNetwork",
            host=host)

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
    def onGetConfigurationFromPast(self):
        info = self.indexInfo()
        device_id = info.get('deviceId')
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

    @Slot()
    def onTimeInformation(self):
        """Get the time information from a `server` or `device`"""
        info = self.indexInfo()
        if info is None:
            return

        def show_server_information(instanceId, success, reply):
            if not success:
                return

            if self.popupWidget is None:
                self.popupWidget = PopupWidget(parent=self)

            actual = Timestamp.fromHashAttributes(reply["time", ...])
            attrs = {"instanceId": instanceId,
                     "Actual Timestamp": actual,
                     "Actual Timing Id": actual.tid}

            # Note: New protocol additions since 2.11
            if "timeServerId" in reply:
                attrs.update({"timeServerId": reply["timeServerId"]})
            if "reference" in reply:
                reference = Timestamp.fromHashAttributes(
                    reply["reference", ...])
                attrs.update({"Last Tick Timestamp": reference,
                              "Last Tick Timing Id": reference.tid})

            self.popupWidget.setInfo(attrs)
            self.popupWidget.adjustSize()
            move_to_cursor(self.popupWidget)
            self.popupWidget.show()

        node_type = info.get('type')
        if node_type is NavigationItemTypes.DEVICE:
            instanceId = info.get('deviceId')
        elif node_type is NavigationItemTypes.SERVER:
            instanceId = info.get('serverId')

        handler = partial(show_server_information, instanceId)
        call_device_slot(handler, instanceId, 'slotGetTime')

    @Slot()
    def onLoggerLevel(self):
        info = self.indexInfo()
        levels = ["DEBUG", "INFO", "WARN", "ERROR"]
        index = levels.index(info.get("log"))
        level, ok = QInputDialog.getItem(self, "Set the logger level", "",
                                         levels, index, False)
        if ok:
            serverId = info.get('serverId')
            network = get_network()
            network.onSetLogLevel(serverId, level)

    @Slot()
    def onKillInstance(self):
        info = self.indexInfo()
        node_type = info.get('type')
        manager = get_manager()

        if node_type is NavigationItemTypes.DEVICE:
            deviceId = info.get('deviceId')
            manager.shutdownDevice(deviceId, parent=self)
        elif node_type is NavigationItemTypes.SERVER:
            serverId = info.get('serverId')
            manager.shutdownServer(serverId, parent=self)

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

    @Slot(QPoint)
    def onCustomContextMenuRequested(self, pos):
        def _test_mask(mask, bit):
            return (mask & bit) == bit

        info = self.indexInfo()
        node_type = info.get('type', NavigationItemTypes.UNDEFINED)
        # Killing services is access level dependent!
        allow_service_edit = access_role_allowed(AccessRole.SERVICE_EDIT)
        if node_type is NavigationItemTypes.HOST:
            instance_control = access_role_allowed(AccessRole.INSTANCE_CONTROL)
            self.acNetworkInfo.setEnabled(instance_control)
            self.mHostItem.exec(QCursor.pos())
        elif node_type is NavigationItemTypes.SERVER:
            self.acKillServer.setEnabled(allow_service_edit)
            self.acLoggerLevel.setEnabled(allow_service_edit)
            _set_tooltip(self.acKillServer, allow_service_edit)
            _set_tooltip(self.acLoggerLevel, allow_service_edit)
            self.acAbout.setVisible(True)
            self.acTimeInformation.setVisible(False)
            self.mServerItem.exec(QCursor.pos())
        elif node_type is NavigationItemTypes.DEVICE:
            self.acKillDevice.setVisible(True)
            self.acKillDevice.setEnabled(allow_service_edit)
            _set_tooltip(self.acKillDevice, allow_service_edit)
            self.acAbout.setVisible(True)
            self.acTimeInformation.setVisible(True)
            has_scenes = _test_mask(info.get('capabilities', 0),
                                    Capabilities.PROVIDES_SCENES)
            self.acOpenScene.setVisible(has_scenes)
            self.mDeviceItem.exec(QCursor.pos())

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
    def onOpenFromFile(self):
        if self._selected_proxy is not None:
            load_configuration_from_file(self._selected_proxy, parent=self)

    @Slot()
    def onSaveToFile(self):
        if self._selected_proxy is not None:
            save_configuration_to_file(self._selected_proxy, parent=self)

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
        self.expandAll()

    def collapseAll(self):
        self.expanded = False
        super().collapseAll()

    def expandAll(self):
        self.expanded = True
        super().expandAll()


def _set_tooltip(action: QAction, enabled: bool) -> None:
    """Add tooltip about lacking acces level when menu-item is disabled."""
    text = action.text() if enabled else DISABLED_TOOLTIP
    action.setToolTip(text)
