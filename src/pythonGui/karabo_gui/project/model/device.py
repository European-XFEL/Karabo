#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from io import StringIO
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance, Property, on_trait_change

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    find_parent_object
)
from karabo.middlelayer_api.newproject.io import (read_project_model,
                                                  write_project_model)
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.project.dialog.object_handle import ObjectDuplicateDialog
from karabo_gui.project.utils import save_object
from karabo_gui.singletons.api import get_manager
from .bases import BaseProjectTreeItem


class DeviceInstanceModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceInstanceModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceInstanceModel)
    active_config = Property(Instance(DeviceConfigurationModel))

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)

        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(partial(self._edit_device,
                                              parent_project))
        config_menu = self._create_sub_menu(menu)
        dupe_action = QAction('Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_device,
                                              parent_project))
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_device,
                                                parent_project))
        save_action = QAction('Save', menu)
        save_action.triggered.connect(self._save_device)
        instantiate_action = QAction('Instantiate', menu)
        instantiate_action.triggered.connect(partial(self._instantiate_device,
                                                     parent_project))
        shutdown_action = QAction('Shutdown', menu)
        shutdown_action.triggered.connect(self.shutdown_device)
        menu.addAction(edit_action)
        menu.addMenu(config_menu)
        menu.addSeparator()
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addAction(save_action)
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

    def _get_active_config(self):
        """ Return the active device configuration object

        :return If a `DeviceConfigurationModel` for the `DeviceInstanceModel`s
        active `uuid` and `revision` can be found, otherwise a `NoneType` is
        returned
        """
        device = self.model
        uuid, revision = device.active_config_ref
        return device.select_config(uuid, revision)

    def _create_sub_menu(self, parent_menu):
        """ Create sub menu for parent menu and return it
        """
        config_menu = QMenu('Configuration', parent_menu)
        add_action = QAction('Add configuration', config_menu)
        config_menu.addAction(add_action)
        active_uuid, active_rev = self.model.active_config_ref
        for dev_conf in self.model.configs:
            text = '{} <{}>'.format(dev_conf.alias, dev_conf.revision)
            conf_action = QAction(text, config_menu)
            conf_action.setCheckable(True)
            dev_conf_ref = (dev_conf.uuid, dev_conf.revision)
            is_active = self.model.active_config_ref == dev_conf_ref
            conf_action.setChecked(is_active)
            config_menu.addAction(conf_action)

        return config_menu

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
            if dev_conf is not None:
                active_config_ref = (dialog.active_uuid,
                                     dialog.active_revision)
                device.active_config_ref = active_config_ref
                dev_conf.description = dialog.description

    def _duplicate_device(self, project):
        """ Duplicate the active device configuration of the model

        :param project: The parent project the model belongs to
        """
        device = self.model
        server_model = find_parent_object(device, project,
                                          DeviceServerModel)
        active_config = self.active_config
        if active_config is None:
            return
        dialog = ObjectDuplicateDialog(device.instance_id)
        if dialog.exec() == QDialog.Accepted:
            xml = write_project_model(active_config)
            for simple_name in dialog.duplicate_names:
                dupe_dev_conf = read_project_model(StringIO(xml))
                # Set a new UUID and revision
                dupe_dev_conf.reset_uuid_and_version()
                dupe_dev_conf.alias = '{}-{}'.format(active_config.alias,
                                                     simple_name)
                config_ref = (dupe_dev_conf.uuid, dupe_dev_conf.revision)
                dev_inst = DeviceInstanceModel(
                    instance_id=simple_name,
                    if_exists=device.if_exists,
                    active_config_ref=config_ref,
                    configs=[dupe_dev_conf]
                )
                server_model.devices.append(dev_inst)

    def _save_device(self):
        active_config = self.active_config
        if active_config is not None:
            save_object(active_config)

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
            get_manager().initDevice(server.server_id, dev_conf.class_id,
                                     device.instance_id, dev_conf.configuration)

    def shutdown_device(self, show_confirm=True):
        device = self.model
        get_manager().shutdownDevice(device.instance_id, show_confirm)
