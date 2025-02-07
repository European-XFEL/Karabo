#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
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

import natsort
from qtpy.QtWidgets import QAction, QDialog, QMenu, QMessageBox
from traits.api import Bool, Instance, Property, on_trait_change

import karabogui.icons as icons
from karabo.common.api import walk_traits_object
from karabo.common.project.api import DeviceServerModel
from karabogui.access import (
    AccessRole, access_role_allowed, get_access_level_for_role)
from karabogui.binding.api import ProxyStatus
from karabogui.dialogs.api import LogDialog
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.indicators import get_project_server_status_icon
from karabogui.itemtypes import ProjectItemTypes
from karabogui.logger import get_logger
from karabogui.project.dialog.server_handle import ServerHandleDialog
from karabogui.project.topo_listener import SystemTopologyListener
from karabogui.project.utils import add_device_to_server
from karabogui.singletons.api import get_manager, get_topology
from karabogui.util import move_to_cursor, version_compatible

from .bases import BaseProjectGroupController, ProjectControllerUiData

ACCESS_LEVEL_TOOLTIP = "Requires minimum '{}' access level"
SERVER_OFFLINE_TOOLTIP = "Server is offline"


class DeviceServerController(BaseProjectGroupController):
    """ A controller for DeviceServerModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceServerModel)
    # Is the server currently online?
    online = Property(Bool, depends_on=['model'])
    # An object which listens to system topology updates
    topo_listener = Instance(SystemTopologyListener)

    def context_menu(self, project_controller, parent=None):
        # Enable/Disable based on online status
        online = self.online

        project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)
        service_allowed = access_role_allowed(AccessRole.SERVICE_EDIT)
        project_edit_access_level = get_access_level_for_role(
            AccessRole.PROJECT_EDIT)
        project_allowed_tooltip = ACCESS_LEVEL_TOOLTIP.format(
            project_edit_access_level)

        service_edit_access_level = get_access_level_for_role(
            AccessRole.SERVICE_EDIT)
        service_allowed_tooltip = ACCESS_LEVEL_TOOLTIP.format(
            service_edit_access_level)
        menu = QMenu(parent)
        menu.setToolTipsVisible(True)
        edit_action = QAction(icons.edit, 'Edit', menu)
        edit_action.triggered.connect(partial(self._edit_server,
                                              parent=parent))
        edit_action.setEnabled(project_allowed)
        if not project_allowed:
            edit_action.setToolTip(project_allowed_tooltip)
        delete_action = QAction(icons.delete, 'Delete', menu)
        delete_action.triggered.connect(partial(self._delete_server,
                                                project_controller,
                                                parent=parent))
        delete_action.setEnabled(project_allowed)
        if not project_allowed:
            delete_action.setToolTip(project_allowed_tooltip)
        shutdown_action = QAction(icons.kill, 'Shutdown', menu)
        shutdown_action.triggered.connect(partial(self._shutdown_server,
                                                  parent=parent))
        shutdown_action.setEnabled(online and service_allowed)
        if not online:
            shutdown_action.setToolTip(SERVER_OFFLINE_TOOLTIP)
        elif not service_allowed:
            shutdown_action.setToolTip(service_allowed_tooltip)
        add_action = QAction(icons.add, 'Add device', menu)
        add_action.setEnabled(online and project_allowed)
        if not online:
            add_action.setToolTip(SERVER_OFFLINE_TOOLTIP)
        elif not project_allowed:
            add_action.setToolTip(project_allowed_tooltip)
        add_action.triggered.connect(partial(self._add_device, parent=parent))

        instantiate_all_action = QAction(
            icons.run, 'Instantiate all devices', menu)
        instantiate_all_action.setEnabled(online and service_allowed)
        if not online:
            instantiate_all_action.setToolTip(SERVER_OFFLINE_TOOLTIP)
        elif not service_allowed:
            instantiate_all_action.setToolTip(service_allowed_tooltip)
        instantiate_all_action.triggered.connect(
            partial(self.instantiate_devices,
                    parent=parent))

        shutdown_all_action = QAction(
            icons.kill, 'Shutdown all devices', menu)
        shutdown_all_action.triggered.connect(partial(self._shutdown_devices,
                                                      parent=parent))
        shutdown_all_action.setEnabled(online and service_allowed)
        if not online:
            shutdown_all_action.setToolTip(SERVER_OFFLINE_TOOLTIP)
        elif not service_allowed:
            shutdown_all_action.setToolTip(service_allowed_tooltip)
        remove_all_action = QAction(icons.delete, 'Delete all devices', menu)
        remove_all_action.triggered.connect(partial(self._delete_all_devices,
                                                    parent=parent))
        remove_all_action.setEnabled(project_allowed)
        if not project_allowed:
            instantiate_all_action.setToolTip(project_allowed_tooltip)

        show_action = QAction(icons.yes, 'Select in topology', menu)
        show_action.triggered.connect(self._show_server)
        show_action.setVisible(self.online)

        menu.addAction(edit_action)
        menu.addAction(delete_action)
        menu.addAction(shutdown_action)
        menu.addSeparator()
        menu.addAction(add_action)
        menu.addAction(instantiate_all_action)
        menu.addAction(shutdown_all_action)
        menu.addAction(remove_all_action)

        # Sub menu for device sorting
        sort_menu = menu.addMenu("&Sort options")
        sort_menu.setIcon(icons.edit)
        sort_menu.setEnabled(project_allowed)
        if not project_allowed:
            sort_menu.setToolTip(project_allowed_tooltip)

        sort_alpha = QAction(
            icons.stringAttribute, 'Sort devices alphabetically', menu)
        sort_alpha.triggered.connect(partial(self._sort_alphabetically,
                                             parent=parent))

        sort_domain = QAction(icons.folderDomain,
                              'Sort devices by domain', menu)
        sort_domain.triggered.connect(partial(self._sort_devices_naming,
                                              level=0, parent=parent))
        sort_type = QAction(icons.folderType,
                            'Sort devices by type', menu)
        sort_type.triggered.connect(partial(self._sort_devices_naming,
                                            level=1, parent=parent))
        sort_member = QAction(icons.deviceInstance,
                              'Sort devices by member', menu)
        sort_member.triggered.connect(partial(self._sort_devices_naming,
                                              level=2, parent=parent))

        sort_menu.addAction(sort_alpha)
        sort_menu.addAction(sort_domain)
        sort_menu.addAction(sort_type)
        sort_menu.addAction(sort_member)

        menu.addAction(show_action)

        return menu

    def create_ui_data(self):
        ui_data = ProjectControllerUiData()
        self._update_icon(ui_data)
        return ui_data

    def system_topology_callback(self, devices, servers):
        """ This callback is called by the ``SystemTopologyListener`` object
        in the ``topo_listener`` trait.
        """
        for server_id, _, status in servers:
            # NOTE: the host is not checked here since it might ot have been
            # set yet and furthermore the server_id is unique in the system
            if self.model.server_id == server_id:
                self.model.status = ProxyStatus(status)

    def _get_display_name(self):
        """Traits property getter for ``display_name``
        """
        return self.model.server_id

    def info(self):
        return {'type': ProjectItemTypes.SERVER,
                'serverId': self.model.server_id,
                'host': self.model.host}

    def delete_press(self, project_controller, parent=None):
        """Reimplemented function on `BaseProjectController`"""
        self._delete_server(project_controller, parent)

    def double_click(self, project_controller, parent=None):
        """Reimplemented function on `BaseProjectController`"""
        server_id = self.model.server_id
        if not self.online:
            get_logger().info(
                f"Cannot retrieve logs for device server <b>{server_id}</b>, "
                "it is <b>offline</b>.")
            return

        attrs = get_topology().get_attributes(f"server.{server_id}")
        assert attrs is not None

        version = attrs.get("karaboVersion", "0.0.0")
        if not version_compatible(version, 2, 14):
            get_logger().info(
                f"Cannot retrieve logs for device server <b>{server_id}</b>. "
                f"Retrieval of logs is available from version 2.14.0.")
            return

        widget = LogDialog(server_id, parent=parent)
        move_to_cursor(widget)
        widget.show()
        widget.raise_()
        widget.activateWindow()

    # ----------------------------------------------------------------------
    # traits handlers

    @on_trait_change("model.modified,model:server_id")
    def update_ui_label(self):
        """ Whenever the project is modified it should be visible to the user
        """
        self._update_icon(self.ui_data)

    @on_trait_change("model.status")
    def status_change(self):
        icon = get_project_server_status_icon(self.model.status)
        if icon is not None:
            self.ui_data.icon = icon

    def _get_online(self):
        return self.model.status is not ProxyStatus.OFFLINE

    def _topo_listener_changed(self, name, old, new):
        """Handle broadcast event registration/unregistration here.
        """
        if old is not None:
            unregister_from_broadcasts(
                {KaraboEvent.SystemTopologyUpdate: old._event_topology})
        if new is not None:
            register_for_broadcasts(
                {KaraboEvent.SystemTopologyUpdate: new._event_topology})

    # ----------------------------------------------------------------------
    # Util methods

    def _update_icon(self, ui_data):
        # Get current status of server
        self.model.status = _get_server_status(self.model.server_id)
        icon = get_project_server_status_icon(self.model.status)
        ui_data.icon = icon

    # ----------------------------------------------------------------------
    # action handlers

    def _show_server(self):
        serverId = self.model.server_id
        broadcast_event(KaraboEvent.ShowDevice,
                        {'deviceId': serverId, 'showTopology': True})

    def _add_device(self, parent=None):
        """Add a device instance to the server
        """
        # NOTE: This can't be connected to the QAction signal, because then
        # a boolean will be passed as the class_id argument
        add_device_to_server(self.model, parent=parent)

    def _delete_server(self, project_controller, parent=None):
        """ Remove the macro associated with this item from its project
        """
        server = self.model
        ask = ('Are you sure you want to delete \"<b>{}</b>\".<br /> '
               'Continue action?'.format(server.simple_name))
        msg_box = QMessageBox(QMessageBox.Question, 'Delete server',
                              ask, QMessageBox.Yes | QMessageBox.No,
                              parent=parent)
        msg_box.setModal(False)
        msg_box.setDefaultButton(QMessageBox.No)
        move_to_cursor(msg_box)
        if msg_box.exec() == QMessageBox.Yes:
            project = project_controller.model
            if server in project.servers:
                del server.devices[:]
                project.servers.remove(server)

    def _edit_server(self, parent=None):
        dialog = ServerHandleDialog(self.model, parent=parent)
        move_to_cursor(dialog)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.server_id = dialog.server_id

    def _shutdown_server(self, parent=None):
        server = self.model
        get_manager().shutdownServer(server.server_id, parent=parent)

    def _delete_all_devices(self, parent=None):
        server = self.model
        ask = ('Do you really want to delete all devices of <br> server '
               '<b>{}</b>?').format(server.simple_name)
        msg_box = QMessageBox(QMessageBox.Question, 'Delete all devices', ask,
                              QMessageBox.Yes | QMessageBox.No, parent=parent)
        msg_box.setModal(False)
        msg_box.setDefaultButton(QMessageBox.No)
        move_to_cursor(msg_box)
        if msg_box.exec() == QMessageBox.No:
            return

        server.devices[:] = []

    def instantiate_devices(self, parent=None):
        """Public action handler to instantiate all devices from this server

        This action handler is also publicly used by the project controller
        """
        server = self.model
        for dev_inst_item in self.children:
            dev_inst_item.instantiate(server, parent)

    def _shutdown_devices(self, parent=None):
        server = self.model
        ask = ('Do you really want to shutdown all devices of <br> server '
               '<b>{}</b>?').format(server.simple_name)
        msg_box = QMessageBox(QMessageBox.Question, 'Shutdown all devices',
                              ask, QMessageBox.Yes | QMessageBox.No,
                              parent=parent)
        msg_box.setModal(False)
        msg_box.setDefaultButton(QMessageBox.No)
        move_to_cursor(msg_box)
        if msg_box.exec() == QMessageBox.No:
            return

        for dev_inst_item in self.children:
            dev_inst_item.shutdown_device(show_confirm=False)

    def _sort_alphabetically(self, parent=None):
        server_model = self.model
        devices = list(server_model.devices)
        devices = natsort.natsorted(devices,
                                    key=lambda device: device.instance_id)
        del server_model.devices[:]
        server_model.devices.extend(devices)

    def _sort_devices_naming(self, level=0, parent=None):
        server_model = self.model
        devices = list(server_model.devices)

        def sort_func(device):
            device_id = device.instance_id
            karabo_name = device_id.split("/")
            return device_id if len(karabo_name) != 3 else karabo_name[level]

        devices = natsort.natsorted(devices, key=sort_func)
        del server_model.devices[:]
        server_model.devices.extend(devices)


# ----------------------------------------------------------------------

def _get_server_status(server_id):
    topology = get_topology()
    attributes = topology.get_attributes(f'server.{server_id}')
    if attributes is not None:
        return ProxyStatus.ONLINE
    return ProxyStatus.OFFLINE


def get_project_servers(project_controller):
    """Given a ``project_controller`` return all the online and offline servers
    """
    online = []
    offline = []

    def visitor(obj):
        if isinstance(obj, DeviceServerController):
            if obj.online:
                online.append(obj)
            else:
                offline.append(obj)

    walk_traits_object(project_controller, visitor)

    return online, offline
