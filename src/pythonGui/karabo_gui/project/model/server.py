#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QMessageBox, QStandardItem
from traits.api import Callable, Dict, Instance, List, on_trait_change

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel)
from karabo.middlelayer import Hash
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.events import (register_for_broadcasts,
                               unregister_from_broadcasts)
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.project.dialog.server_handle import ServerHandleDialog
from karabo_gui.project.topo_listener import SystemTopologyListener
from karabo_gui.project.utils import save_object
from karabo_gui.singletons.api import get_manager
from .bases import BaseProjectTreeItem
from .device import DeviceInstanceModelItem


class DeviceServerModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceServerModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceServerModel)
    # A factory for shadow items wrapping children
    child_create = Callable
    # A callable which can gracefully destroy a child shadow object
    child_destroy = Callable
    # Different devices for the server
    children = List(Instance(DeviceInstanceModelItem))
    _child_map = Dict  # dictionary for fast lookups during removal
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
        add_action = QAction('Add device configuration', menu)
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
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.yes)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)

        return item

    def item_handler(self, event):
        """ Called for List-trait events on ``model`` (a DeviceServerModel)

        This notification handler is connected and disconnected in the
        create_device_server_model_shadow and
        destroy_device_server_model_shadow functions.
        """
        removals = []
        for model in event.removed:
            item_model = self._child_map[model]
            self.children.remove(item_model)
            self.child_destroy(item_model)
            removals.append(item_model)

        additions = [self.child_create(model=model) for model in event.added]
        self.children.extend(additions)

        # Synchronize the GUI with the Traits model
        self._update_ui_children(additions, removals)

    def system_topology_callback(self, added, removed):
        """ This callback is called by the ``SystemTopologyListener`` object
        in the ``topo_listener`` trait.
        """
        pass

    def _children_items_changed(self, event):
        """ Maintain ``_child_map`` by watching item events on ``children``

        This is a static notification handler which is connected automatically
        by Traits.
        """
        for item_model in event.removed:
            del self._child_map[item_model.model]

        for item_model in event.added:
            self._child_map[item_model.model] = item_model

    def _update_ui_children(self, additions, removals):
        """ Propagate changes from the Traits model to the Qt item model.
        """
        def _find_child_qt_item(item_model):
            for i in range(self.qt_item.rowCount()):
                row_child = self.qt_item.child(i)
                row_model = row_child.data(PROJECT_ITEM_MODEL_REF)()
                if row_model is item_model:
                    return i
            return -1

        # Stop immediately if the UI is not yet initialized
        if not self.is_ui_initialized():
            return

        for item in removals:
            index = _find_child_qt_item(item)
            if index >= 0:
                self.qt_item.removeRow(index)

        for item in additions:
            self.qt_item.appendRow(item.qt_item)

    @on_trait_change("model.server_id")
    def server_id_change(self):
        if not self.is_ui_initialized():
            return
        self.qt_item.setText(self.model.server_id)

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
                alias=dialog.alias, description=dialog.description,
                initialized=True
            )
            active_config_ref = (config_model.uuid, config_model.revision)
            traits = {
                'instance_id': dialog.instance_id,
                'if_exists': dialog.if_exists,
                'configs': [config_model],
                'active_config_ref': active_config_ref
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
