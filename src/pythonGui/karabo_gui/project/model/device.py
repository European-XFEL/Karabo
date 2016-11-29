#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance, on_trait_change

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    find_parent_object
)
from karabo.middlelayer import Hash
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.singletons.api import get_manager
from .bases import BaseProjectTreeItem


class DeviceInstanceModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceInstanceModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceInstanceModel)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)

        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(partial(self._edit_device,
                                              parent_project))
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_device,
                                                parent_project))
        instantiate_action = QAction('Instantiate', menu)
        instantiate_action.triggered.connect(partial(self._instantiate_device,
                                                     parent_project))
        shutdown_action = QAction('Shutdown', menu)
        shutdown_action.triggered.connect(self.shutdown_device)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(instantiate_action)
        menu.addAction(shutdown_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.instance_id)
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.deviceClass)
        item.setEditable(False)
        return item

    @on_trait_change("model.instance_id")
    def instance_id_change(self):
        if not self.is_ui_initialized():
            return
        self.qt_item.setText(self.model.instance_id)

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_device(self, project):
        """ Remove the device associated with this item from its device server
        """
        device = self.model
        server_model = find_parent_object(device, project,
                                          DeviceServerModel)
        if device in server_model.devices:
            server_model.devices.remove(device)

    def _edit_device(self, project):
        device = self.model
        server_model = find_parent_object(device, project,
                                          DeviceServerModel)
        dialog = DeviceHandleDialog(server_id=server_model.server_id,
                                    model=device)
        result = dialog.exec()
        if result == QDialog.Accepted:
            device.instance_id = dialog.instance_id
            device.if_exists = dialog.if_exists

            # Look for existing DeviceConfigurationModel
            dev_conf = device.select_config(dialog.active_uuid,
                                            dialog.active_revision)
            if dialog.new_config or dev_conf is None:
                dev_conf = DeviceConfigurationModel(
                    class_id=dialog.class_id, configuration=Hash(),
                    description=dialog.description,
                    initialized=True
                )
                device.configs.append(dev_conf)
                device.active_config_ref = (dev_conf.uuid, dev_conf.revision)
            else:
                active_config_ref = (dialog.active_uuid, dialog.active_revision)
                device.active_config_ref = active_config_ref
                dev_conf.description = dialog.description

    def _instantiate_device(self, project):
        server = find_parent_object(self.model, project, DeviceServerModel)
        self.instantiate(server)

    def instantiate(self, server):
        """ Instantiate this device instance on the given `server`

        :param server: The server this device belongs to
        """
        device = self.model
        uuid, revision = device.active_config_ref
        dev_conf = device.select_config(uuid, revision)
        if dev_conf is not None:
            server_id = server.server_id
            instance_id = device.instance_id
            class_id = dev_conf.class_id
            config = dev_conf.configuration
            get_manager().initDevice(server_id, class_id, instance_id, config)

    def shutdown_device(self, show_confirm=True):
        device = self.model
        get_manager().shutdownDevice(device.instance_id, show_confirm)
