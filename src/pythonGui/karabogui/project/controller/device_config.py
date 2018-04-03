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
from karabogui.project.dialog.object_handle import ObjectEditDialog
from karabogui.project.utils import check_device_config_exists
from .bases import BaseProjectController, ProjectControllerUiData


class DeviceConfigurationController(BaseProjectController):
    """ A controller for DeviceConfigurationModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceConfigurationModel)
    initial_check_state = Int(Qt.Unchecked)

    def context_menu(self, project_controller, parent=None):
        menu = QMenu(parent)

        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(partial(self._edit_config,
                                              project_controller))

        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_config,
                                                project_controller))

        menu.addAction(edit_action)
        menu.addAction(delete_action)
        return menu

    def create_ui_data(self):
        return ProjectControllerUiData(checkable=True,
                                       check_state=self.initial_check_state)

    # ----------------------------------------------------------------------
    # QAction handlers

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
