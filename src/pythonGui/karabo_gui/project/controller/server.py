#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QMessageBox, QStandardItem
from traits.api import Instance, on_trait_change

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel)
from karabo.middlelayer import Hash
from karabo_gui.const import PROJECT_CONTROLLER_REF
from karabo_gui.events import (register_for_broadcasts,
                               unregister_from_broadcasts)
from karabo_gui.indicators import DeviceStatus, get_project_server_status_icon
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.project.dialog.server_handle import ServerHandleDialog
from karabo_gui.project.topo_listener import SystemTopologyListener
from karabo_gui.project.utils import save_object
from karabo_gui.singletons.api import get_manager, get_topology
from .bases import BaseProjectGroupController


class DeviceServerController(BaseProjectGroupController):
    """ A controller for DeviceServerModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceServerModel)
    # An object which listens to system topology updates
    topo_listener = Instance(SystemTopologyListener)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_server)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_server,
                                                parent_project))
        save_action = QAction('Save', menu)
        save_action.triggered.connect(partial(save_object, self.model))
        shutdown_action = QAction('Shutdown', menu)
        shutdown_action.triggered.connect(self._shutdown_server)
        add_action = QAction('Add device', menu)
        add_action.triggered.connect(self._add_device)
        instantiate_all_action = QAction('Instantiate all devices', menu)
        instantiate_all_action.triggered.connect(self._instantiate_devices)
        shutdown_all_action = QAction('Shutdown all devices', menu)
        shutdown_all_action.triggered.connect(self._shutdown_devices)
        remove_all_action = QAction('Delete all devices', menu)
        remove_all_action.triggered.connect(self._delete_all_devices)
        menu.addAction(edit_action)
        menu.addAction(delete_action)
        menu.addAction(save_action)
        menu.addAction(shutdown_action)
        menu.addSeparator()
        menu.addAction(add_action)
        menu.addAction(instantiate_all_action)
        menu.addAction(shutdown_all_action)
        menu.addAction(remove_all_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.server_id)
        item.setData(weakref.ref(self), PROJECT_CONTROLLER_REF)
        # Get current status of server
        self.model.status = _get_server_status(self.model.server_id)
        icon = get_project_server_status_icon(DeviceStatus(self.model.status))
        item.setIcon(icon)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)
        self.set_qt_item_text(item, self.model.simple_name)
        return item

    def system_topology_callback(self, devices, servers):
        """ This callback is called by the ``SystemTopologyListener`` object
        in the ``topo_listener`` trait.
        """
        for server_id, host, status in servers:
            if self.model.server_id == server_id and self.model.host == host:
                self.model.status = status

    # ----------------------------------------------------------------------
    # traits notification handlers

    @on_trait_change("model.modified,model.simple_name")
    def update_ui_label(self):
        """ Whenever the project is modified it should be visible to the user
        """
        if not self.is_ui_initialized():
            return
        self.set_qt_item_text(self.qt_item, self.model.simple_name)

    @on_trait_change("model.status")
    def status_change(self):
        if not self.is_ui_initialized():
            return
        status_enum = DeviceStatus(self.model.status)
        icon = get_project_server_status_icon(status_enum)
        if icon is not None:
            self.qt_item.setIcon(icon)

    def _topo_listener_changed(self, name, old, new):
        """Handle broadcast event registration/unregistration here.
        """
        if old is not None:
            unregister_from_broadcasts(old)
        if new is not None:
            register_for_broadcasts(new)

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_server(self, project):
        """ Remove the macro associated with this item from its project
        """
        server = self.model
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

    def _add_device(self):
        """ Add a device to this server
        """
        dialog = DeviceHandleDialog(server_id=self.model.server_id)
        result = dialog.exec()
        if result == QDialog.Accepted:
            config_model = DeviceConfigurationModel(
                class_id=dialog.class_id, configuration=Hash(),
                simple_name=dialog.alias, alias=dialog.alias,
                description=dialog.description, initialized=True, modified=True
            )
            active_config_ref = (config_model.uuid, config_model.revision)
            traits = {
                'class_id': dialog.class_id,
                'instance_id': dialog.instance_id,
                'if_exists': dialog.if_exists,
                'configs': [config_model],
                'active_config_ref': active_config_ref,
                'initialized': True,
                'modified': True,
            }
            device = DeviceInstanceModel(**traits)
            self.model.devices.append(device)

    def _delete_all_devices(self):
        server = self.model
        ask = ('Do you really want to delete all devices of server '
               '\"<b>{}</b>\"?').format(server.simple_name)
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
        for dev_inst_item in self.children:
            dev_inst_item.shutdown_device(show_confirm=False)


# ----------------------------------------------------------------------

def _get_server_status(server_id):
    topology = get_topology()
    attributes = topology.get_attributes('server.{}'.format(server_id))
    if attributes is not None:
        return attributes.get('status', 'ok')
    return 'offline'
