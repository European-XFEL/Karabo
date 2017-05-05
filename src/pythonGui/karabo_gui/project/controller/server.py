#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtGui import QAction, QDialog, QMenu, QMessageBox
from traits.api import Bool, Instance, Property, on_trait_change

from karabo.common.project.api import DeviceServerModel
from karabo_gui.events import (register_for_broadcasts,
                               unregister_from_broadcasts)
from karabo_gui.indicators import DeviceStatus, get_project_server_status_icon
from karabo_gui.project.dialog.server_handle import ServerHandleDialog
from karabo_gui.project.topo_listener import SystemTopologyListener
from karabo_gui.project.utils import add_device_to_server
from karabo_gui.singletons.api import get_manager, get_topology
from .bases import BaseProjectGroupController, ProjectControllerUiData


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

        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_server)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_server,
                                                project_controller))
        shutdown_action = QAction('Shutdown', menu)
        shutdown_action.setEnabled(online)
        shutdown_action.triggered.connect(self._shutdown_server)
        add_action = QAction('Add device', menu)
        add_action.setEnabled(online)
        add_action.triggered.connect(self._add_device)
        instantiate_all_action = QAction('Instantiate all devices', menu)
        instantiate_all_action.setEnabled(online)
        instantiate_all_action.triggered.connect(self._instantiate_devices)
        shutdown_all_action = QAction('Shutdown all devices', menu)
        shutdown_all_action.triggered.connect(self._shutdown_devices)
        remove_all_action = QAction('Delete all devices', menu)
        remove_all_action.triggered.connect(self._delete_all_devices)
        menu.addAction(edit_action)
        menu.addAction(delete_action)
        menu.addAction(shutdown_action)
        menu.addSeparator()
        menu.addAction(add_action)
        menu.addAction(instantiate_all_action)
        menu.addAction(shutdown_all_action)
        menu.addAction(remove_all_action)
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
                self.model.status = status

    def _get_display_name(self):
        """Traits property getter for ``display_name``
        """
        return self.model.server_id

    # ----------------------------------------------------------------------
    # traits handlers

    @on_trait_change("model.modified,model:server_id")
    def update_ui_label(self):
        """ Whenever the project is modified it should be visible to the user
        """
        self._update_icon(self.ui_data)

    @on_trait_change("model.status")
    def status_change(self):
        status_enum = DeviceStatus(self.model.status)
        icon = get_project_server_status_icon(status_enum)
        if icon is not None:
            self.ui_data.icon = icon

    def _get_online(self):
        return DeviceStatus(self.model.status) != DeviceStatus.STATUS_OFFLINE

    def _topo_listener_changed(self, name, old, new):
        """Handle broadcast event registration/unregistration here.
        """
        if old is not None:
            unregister_from_broadcasts(old)
        if new is not None:
            register_for_broadcasts(new)

    # ----------------------------------------------------------------------
    # Util methods

    def _update_icon(self, ui_data):
        # Get current status of server
        self.model.status = _get_server_status(self.model.server_id)
        icon = get_project_server_status_icon(DeviceStatus(self.model.status))
        ui_data.icon = icon

    # ----------------------------------------------------------------------
    # action handlers

    def _add_device(self):
        """Add a device instance to the server
        """
        # NOTE: This can't be connected to the QAction signal, because then
        # a boolean will be passed as the class_id argument
        add_device_to_server(self.model)

    def _delete_server(self, project_controller):
        """ Remove the macro associated with this item from its project
        """
        server = self.model
        project = project_controller.model
        if server in project.servers:
            project.servers.remove(server)

    def _edit_server(self):
        dialog = ServerHandleDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.server_id = dialog.server_id
            self.model.host = dialog.host
            self.model.description = dialog.description

    def _shutdown_server(self):
        server = self.model
        get_manager().shutdownServer(server.server_id)

    def _delete_all_devices(self):
        server = self.model
        ask = ('Do you really want to delete all devices of <br> server '
               '<b>{}</b>?').format(server.simple_name)
        reply = QMessageBox.question(None, 'Delete all devices', ask,
                                     QMessageBox.Yes | QMessageBox.No,
                                     QMessageBox.No)
        if reply == QMessageBox.No:
            return

        server.devices[:] = []

    def _instantiate_devices(self):
        server = self.model
        for dev_inst_item in self.children:
            dev_inst_item.instantiate(server)

    def _shutdown_devices(self):
        server = self.model
        ask = ('Do you really want to shutdown all devices of <br> server '
               '<b>{}</b>?').format(server.simple_name)
        reply = QMessageBox.question(None, 'Shutdown all devices', ask,
                                     QMessageBox.Yes | QMessageBox.No,
                                     QMessageBox.No)
        if reply == QMessageBox.No:
            return

        for dev_inst_item in self.children:
            dev_inst_item.shutdown_device(show_confirm=False)


# ----------------------------------------------------------------------

def _get_server_status(server_id):
    topology = get_topology()
    attributes = topology.get_attributes('server.{}'.format(server_id))
    if attributes is not None:
        return attributes.get('status', 'ok')
    return 'offline'
