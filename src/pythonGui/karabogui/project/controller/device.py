#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from io import StringIO

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QDialog, QMenu
from traits.api import Instance, Property, on_trait_change

from karabo.common.api import DeviceStatus, NO_CONFIG_STATUSES
from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    find_parent_object
)
from karabo.middlelayer import Hash
from karabo.middlelayer_api.project.api import (read_project_model,
                                                write_project_model)
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.indicators import get_project_device_status_icon
from karabogui.project.dialog.device_handle import DeviceHandleDialog
from karabogui.project.dialog.object_handle import ObjectDuplicateDialog
from karabogui.project.utils import (
    check_device_config_exists, check_device_instance_exists)
from karabogui.singletons.api import get_manager, get_topology
from karabogui.topology.api import ProjectDeviceInstance
from .bases import BaseProjectGroupController, ProjectControllerUiData
from .server import DeviceServerController


class DeviceInstanceController(BaseProjectGroupController):
    """ A wrapper for DeviceInstanceModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceInstanceModel)
    active_config = Property(Instance(DeviceConfigurationModel))

    # A proxy for online and offline device configurations
    project_device = Instance(ProjectDeviceInstance)

    def context_menu(self, project_controller, parent=None):
        menu = QMenu(parent)

        server_controller = find_parent_object(self, project_controller,
                                               DeviceServerController)
        server_online = server_controller.online
        proj_device_online = self.project_device.online
        proj_device_status = self.project_device.proxy.status

        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(partial(self._edit_device,
                                              project_controller))
        config_menu = self._create_sub_menu(menu, project_controller)
        dupe_action = QAction('Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_device,
                                              project_controller))
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_device,
                                                project_controller))
        instantiate_action = QAction('Instantiate', menu)
        can_instantiate = (server_online and not proj_device_online and
                           proj_device_status not in NO_CONFIG_STATUSES)
        instantiate_action.setEnabled(can_instantiate)
        instantiate_action.triggered.connect(partial(self._instantiate_device,
                                                     project_controller))
        shutdown_action = QAction('Shutdown', menu)
        shutdown_action.setEnabled(proj_device_online)
        shutdown_action.triggered.connect(partial(self.shutdown_device,
                                                  show_confirm=True))
        menu.addAction(edit_action)
        menu.addMenu(config_menu)
        menu.addSeparator()
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(instantiate_action)
        menu.addAction(shutdown_action)
        return menu

    def create_ui_data(self):
        ui_data = ProjectControllerUiData()
        self._update_icon(ui_data)
        return ui_data

    def single_click(self, project_controller, parent=None):
        if not self.model.initialized:
            return

        active_config = self.active_config
        if active_config is None:
            return

        self._broadcast_item_click()

    def _get_display_name(self):
        """Traits property getter for ``display_name``
        """
        return self.model.instance_id

    # ----------------------------------------------------------------------
    # Traits handlers

    def _get_active_config(self):
        """Traits Property getter for the active device configuration object

        :return If a `DeviceConfigurationModel` for the `DeviceInstanceModel`s
        active `uuid` can be found, otherwise a `NoneType` is returned
        """
        device = self.model
        return device.select_config(device.active_config_ref)

    def _project_device_default(self):
        """Traits default initializer for `project_device`.
        """
        device = self.model
        config = self.active_config

        # Complain loudly if this trait is initialized when the model isn't
        assert device.initialized, "DeviceInstanceModel must be initialized!"
        assert config.initialized, "Device config must be initialized!"

        self._update_check_state()

        return get_topology().get_project_device(
            device.instance_id, device.server_id, device.class_id,
            None if config is None else config.configuration
        )

    @on_trait_change("model:instance_id,model:server_id,model:class_id")
    def _forced_project_device_change(self):
        """When instance_id/server_id/class_id changes, the project device
        needs to be notified
        """
        # Watch for incomplete model initialization
        if not self.model.initialized:
            return

        # Tell the project device
        self.project_device.rename(
            device_id=self.model.instance_id,
            server_id=self.model.server_id,
            class_id=self.model.class_id
        )

    @on_trait_change("model:initialized,model:instance_id")
    def _update_ui_label(self):
        """ Whenever the object is modified it should be visible to the user
        """
        self._update_icon(self.ui_data)
        self._update_alarm_type()

    @on_trait_change("model:active_config_ref")
    def _active_config_ref_change(self):
        """Called when user selects a different configuration in project panel
        """
        # Watch for incomplete model and view initialization
        if not self.model.initialized:
            return

        config = self.active_config
        if config is not None:
            self.project_device.set_project_config_hash(config.configuration)

        self._update_check_state()

    @on_trait_change('project_device:proxy.binding.config_update')
    def _active_config_changed_in_configurator(self):
        """Called whenever project device current configuration is changed
        """
        if self.project_device.online:
            return
        config = self.active_config
        if config is not None:
            new_config = self.project_device.get_current_config_hash()
            config.configuration = new_config

    @on_trait_change("project_device:status")
    def status_change(self, status):
        self._update_icon(self.ui_data)
        if status not in (DeviceStatus.NOPLUGIN, DeviceStatus.NOSERVER):
            # Show the device's configuration, iff it was already showing
            self._update_configurator()

    @on_trait_change("project_device:device_node.alarm_info.alarm_dict_items")
    def _alarm_info_change(self):
        self._update_alarm_type()

    # ----------------------------------------------------------------------
    # Util methods

    def active_config_changed(self, config_model):
        """Whenever the active config is changed from the context menu, update
        the project device proxy
        """
        if not self.model.initialized or config_model.configuration is None:
            return

        device = self.model
        # changing model's active_config_ref will trigger trait change handler
        device.active_config_ref = config_model.uuid
        # Notify configurator to display the new configuration,
        # ProjectDeviceInstance doesn't broadcast anything to configurator
        self._broadcast_item_click()

    def _broadcast_item_click(self):
        broadcast_event(KaraboEventSender.ShowConfiguration,
                        {'proxy': self.project_device.proxy})

    def _update_icon(self, ui_data):
        # Get current status of device
        if not self.model.initialized:
            return

        status_enum = self.project_device.status
        ui_data.icon = get_project_device_status_icon(status_enum)
        ui_data.status = status_enum

    def _update_check_state(self):
        """Update the Qt.CheckState of the ``DeviceConfigurationController``
        children
        """
        if not self.model.initialized:
            return

        active_config = self.active_config
        for child in self.children:
            check_state = (Qt.Checked if active_config is child.model
                           else Qt.Unchecked)
            child.ui_data.check_state = check_state

    def _update_alarm_type(self):
        # Get current status of device
        if not self.model.initialized:
            return

        device_node = self.project_device.device_node
        if device_node is None:
            alarm_type = ''
        else:
            alarm_type = device_node.alarm_info.alarm_type
        self.ui_data.alarm_type = alarm_type

    def _update_configurator(self):
        broadcast_event(KaraboEventSender.UpdateDeviceConfigurator,
                        {'proxy': self.project_device.proxy})

    def _create_sub_menu(self, parent_menu, project_controller):
        """ Create sub menu for parent menu and return it
        """
        config_menu = QMenu('Configuration', parent_menu)
        add_action = QAction('Add device configuration', config_menu)
        add_action.triggered.connect(partial(self._add_configuration,
                                             project_controller))
        config_menu.addAction(add_action)
        for dev_conf in self.model.configs:
            conf_action = QAction(dev_conf.simple_name, config_menu)
            conf_action.setCheckable(True)
            callback = partial(self.active_config_changed, dev_conf)
            conf_action.triggered.connect(callback)
            is_active = self.model.active_config_ref == dev_conf.uuid
            conf_action.setChecked(is_active)
            config_menu.addAction(conf_action)

        prj_dev = self.project_device
        if prj_dev.status in NO_CONFIG_STATUSES or prj_dev.online:
            config_menu.setEnabled(False)

        return config_menu

    # ----------------------------------------------------------------------
    # QAction handlers

    def _delete_device(self, project_controller):
        """ Remove the device associated with this item from its device server
        """
        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)
        if device in server_model.devices:
            server_model.devices.remove(device)

    def _edit_device(self, project_controller):
        # Watch for incomplete model initialization
        if not self.model.initialized:
            return

        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)
        dialog = DeviceHandleDialog(server_id=server_model.server_id,
                                    model=device,
                                    is_online=self.project_device.online)
        result = dialog.exec()
        if result == QDialog.Accepted:
            # Check for existing device
            renamed = device.instance_id != dialog.instance_id
            if renamed and check_device_instance_exists(dialog.instance_id):
                return

            device.instance_id = dialog.instance_id
            device.if_exists = dialog.if_exists

            # Look for existing DeviceConfigurationModel
            dev_conf = device.select_config(dialog.active_uuid)
            if dev_conf is not None:
                device.active_config_ref = dialog.active_uuid
                dev_conf.class_id = dialog.class_id
                dev_conf.description = dialog.description

    def _add_configuration(self, project_controller):
        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)
        dialog = DeviceHandleDialog(server_id=server_model.server_id,
                                    model=device, add_config=True)
        result = dialog.exec()
        if result == QDialog.Accepted:
            # Check for existing device configuration
            if check_device_config_exists(dialog.configuration_name):
                return

            config_model = DeviceConfigurationModel(
                class_id=dialog.class_id, configuration=Hash(),
                simple_name=dialog.configuration_name,
                description=dialog.description
            )
            # Set initialized and modified last
            config_model.initialized = config_model.modified = True
            device.configs.append(config_model)
            device.active_config_ref = config_model.uuid

    def _duplicate_device(self, project_controller):
        """ Duplicate the active device configuration of the model

        :param project: The parent project controller the model belongs to
        """
        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)
        active_config = self.active_config
        if active_config is None:
            return

        dialog = ObjectDuplicateDialog(device.instance_id)
        if dialog.exec() == QDialog.Accepted:
            xml = write_project_model(active_config)
            for simple_name in dialog.duplicate_names:
                # Check for existing device
                if check_device_instance_exists(simple_name):
                    continue
                dupe_dev_conf = read_project_model(StringIO(xml))
                dupe_dev_conf.reset_uuid()
                dev_inst = DeviceInstanceModel(
                    class_id=device.class_id,
                    instance_id=simple_name,
                    if_exists=device.if_exists,
                    active_config_ref=dupe_dev_conf.uuid,
                    configs=[dupe_dev_conf]
                )
                # Set initialized and modified last
                dev_inst.initialized = dev_inst.modified = True
                server_model.devices.append(dev_inst)

    def _instantiate_device(self, project_controller):
        server = find_parent_object(self.model, project_controller.model,
                                    DeviceServerModel)
        self.instantiate(server)

    def instantiate(self, server):
        """ Instantiate this device instance on the given `server`

        :param server: The server this device belongs to
        """
        device = self.model
        dev_conf = device.select_config(device.active_config_ref)
        if dev_conf is not None:
            get_manager().initDevice(server.server_id, dev_conf.class_id,
                                     device.instance_id,
                                     dev_conf.configuration)

    def shutdown_device(self, show_confirm=True):
        device = self.model
        get_manager().shutdownDevice(device.instance_id, show_confirm)
