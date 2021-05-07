#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAction, QDialog, QMenu
from traits.api import Instance, Int

import karabogui.icons as icons
from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, find_parent_object)
from karabo.native import Hash
from karabogui import messagebox
from karabogui.access import AccessRole, access_role_allowed
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.itemtypes import ProjectItemTypes
from karabogui.project.dialog.device_configuration import (
    DeviceConfigurationDialog)
from karabogui.project.dialog.object_handle import ObjectEditDialog
from karabogui.project.utils import check_device_config_exists
from karabogui.util import move_to_cursor

from .bases import BaseProjectController, ProjectControllerUiData
from .device import DeviceInstanceController

DEFAULT = 'default'


class DeviceConfigurationController(BaseProjectController):
    """ A controller for DeviceConfigurationModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceConfigurationModel)
    initial_check_state = Int(Qt.Checked)

    def context_menu(self, project_controller, parent=None):
        config = self.model
        device_controller = find_parent_object(self, project_controller,
                                               DeviceInstanceController)
        # Check if the device we belong to is online!
        proxy = device_controller.project_device.proxy

        project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)
        conf_allowed = project_allowed and config.simple_name != DEFAULT
        menu = QMenu(parent)

        edit_action = QAction(icons.edit, 'Edit', menu)
        edit_action.triggered.connect(partial(self._edit_config,
                                              project_controller,
                                              parent=parent))
        edit_action.setEnabled(conf_allowed)

        delete_action = QAction(icons.delete, 'Delete', menu)
        delete_action.triggered.connect(partial(self._delete_config,
                                                project_controller,
                                                parent=parent))
        delete_action.setEnabled(conf_allowed)

        conf_action = QAction(icons.load, 'Load in Configurator', menu)
        conf_action.triggered.connect(partial(self._load_config,
                                              project_controller,
                                              parent=parent))
        conf_action.setEnabled(proxy.online)

        store_conf_action = QAction('Stored configuration', menu)
        store_conf_action.triggered.connect(partial(self._show_config,
                                                    project_controller,
                                                    parent=parent))

        menu.addAction(edit_action)
        menu.addAction(delete_action)
        menu.addAction(conf_action)
        menu.addAction(store_conf_action)

        return menu

    def create_ui_data(self):
        return ProjectControllerUiData(checkable=True,
                                       check_state=self.initial_check_state)

    def info(self):
        return {'type': ProjectItemTypes.CONFIGURATION,
                'classId': self.model.class_id,
                'configuration': self.model.configuration}

    # ----------------------------------------------------------------------
    # QAction handlers

    def _show_config(self, project_controller, parent=None):
        device_controller = find_parent_object(self, project_controller,
                                               DeviceInstanceController)
        project_device = device_controller.project_device

        model = self.model
        config = model.configuration
        dialog = DeviceConfigurationDialog(
            name=model.simple_name, configuration=config,
            project_device=project_device, parent=parent)
        move_to_cursor(dialog)
        if dialog.exec_() == QDialog.Accepted:
            self.model.configuration = dialog.configuration
            project_device.set_project_config_hash(dialog.configuration)
            proxy = project_device.proxy
            if proxy.online:
                return
            # Show the device configuration if online and already showing
            broadcast_event(KaraboEvent.UpdateDeviceConfigurator,
                            {'proxy': proxy})

    def _load_config(self, project_controller, parent=None):
        """Send a configuration to the configurator

        The configuration is build from the class schema of the project device
        Afterwards, the user-edits of the selected configuration are merged
        into the configuration which is sent to the configurator
        """
        device_controller = find_parent_object(self, project_controller,
                                               DeviceInstanceController)
        project_device = device_controller.project_device
        config = project_device.get_schema_default_configuration()
        if config.empty():
            message = ("The schema configuration of this project "
                       "device cannot be retrieved!")
            messagebox.show_error(message, parent=parent)
            return

        config.merge(self.model.configuration)
        # Build a standard gui configuration with class id and config Hash
        class_id = self.model.class_id
        configuration = Hash(class_id, config)

        # The online proxy of the project project_device
        proxy = project_device.proxy
        # Broadcast to configurator!
        data = {'proxy': proxy, 'configuration': configuration}
        broadcast_event(KaraboEvent.LoadConfiguration, data)

    def _delete_config(self, project_controller, parent=None):
        """ Remove the device associated with this item from its device server
        """
        config = self.model
        device_model = find_parent_object(config, project_controller.model,
                                          DeviceInstanceModel)

        # Device model needs at least one configuration
        if len(device_model.configs) == 1:
            msg = ('Removal not supported.<br><br>A device needs to have at '
                   'least one configuration.')
            messagebox.show_warning(msg)
            return

        # Only allow removing of none active configurations
        if config.uuid == device_model.active_config_ref:
            msg = ('This is currently the active configuration.<br><br>Please '
                   'define another active configuration.')
            messagebox.show_warning(msg, parent=parent)
            return

        if config in device_model.configs:
            device_model.configs.remove(config)

    def _edit_config(self, project_controller, parent=None):
        config = self.model
        device_model = find_parent_object(config, project_controller.model,
                                          DeviceInstanceModel)
        instance_id = device_model.instance_id

        dialog = ObjectEditDialog(object_type='device configuration',
                                  model=config, parent=parent)
        move_to_cursor(dialog)
        result = dialog.exec()
        if result == QDialog.Accepted:
            # Check for existing device configuration
            renamed = self.model.simple_name != dialog.simple_name
            if renamed and check_device_config_exists(instance_id,
                                                      dialog.simple_name):
                return

            config.simple_name = dialog.simple_name
