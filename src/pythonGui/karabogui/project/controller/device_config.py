#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QDialog, QMenu
from traits.api import Instance, Int

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, find_parent_object)
from karabogui import messagebox
from karabogui.enums import ProjectItemTypes
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.project.dialog.object_handle import ObjectEditDialog
from karabogui.project.utils import check_device_config_exists
from karabo.native import Hash
from .bases import BaseProjectController, ProjectControllerUiData
from .device import DeviceInstanceController

DEFAULT = 'default'


class DeviceConfigurationController(BaseProjectController):
    """ A controller for DeviceConfigurationModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceConfigurationModel)
    initial_check_state = Int(Qt.Unchecked)

    def context_menu(self, project_controller, parent=None):
        config = self.model
        device_controller = find_parent_object(self, project_controller,
                                               DeviceInstanceController)
        # Check if the device we belong to is online!
        proxy = device_controller.project_device.proxy

        menu = QMenu(parent)

        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(partial(self._edit_config,
                                              project_controller))
        edit_action.setEnabled(config.simple_name != DEFAULT)

        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_config,
                                                project_controller))
        delete_action.setEnabled(config.simple_name != DEFAULT)

        conf_action = QAction('Load in Configurator', menu)
        conf_action.triggered.connect(partial(self._load_config,
                                              project_controller))
        conf_action.setEnabled(proxy.online)

        menu.addAction(edit_action)
        menu.addAction(delete_action)
        menu.addAction(conf_action)

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

    def _load_config(self, project_controller):
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
            messagebox.show_error("The schema configuration of this project "
                                  "device cannot be retrieved!")
            return

        config.merge(self.model.configuration)
        # Build a standard gui configuration with class id and config Hash
        class_id = self.model.class_id
        configuration = Hash(class_id, config)

        # The online proxy of the project project_device
        proxy = project_device.proxy
        # Broadcast to configurator!
        data = {'proxy': proxy, 'configuration': configuration}
        broadcast_event(KaraboEventSender.LoadConfiguration, data)

    def _delete_config(self, project_controller):
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
            messagebox.show_warning(msg)
            return

        if config in device_model.configs:
            device_model.configs.remove(config)

    def _edit_config(self, project_controller):
        config = self.model
        device_model = find_parent_object(config, project_controller.model,
                                          DeviceInstanceModel)
        instance_id = device_model.instance_id

        dialog = ObjectEditDialog(object_type='device configuration',
                                  model=config)
        result = dialog.exec()
        if result == QDialog.Accepted:
            # Check for existing device configuration
            renamed = self.model.simple_name != dialog.simple_name
            if renamed and check_device_config_exists(instance_id,
                                                      dialog.simple_name):
                return

            config.simple_name = dialog.simple_name
