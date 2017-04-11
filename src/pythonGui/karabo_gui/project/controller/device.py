#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from io import StringIO

from PyQt4.QtGui import QAction, QDialog, QMenu
from traits.api import Instance, Property, on_trait_change

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    find_parent_object
)
from karabo.middlelayer import Hash
from karabo.middlelayer_api.project.api import (read_project_model,
                                                write_project_model)
from karabo_gui.events import broadcast_event, KaraboEventSender
from karabo_gui.indicators import DeviceStatus, get_project_device_status_icon
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.project.dialog.object_handle import ObjectDuplicateDialog
from karabo_gui.project.utils import (
    update_check_state, check_device_instance_exists)
from karabo_gui.singletons.api import get_manager, get_topology
from karabo_gui.topology.api import ProjectDeviceInstance
from .bases import BaseProjectGroupController, ProjectControllerUiData


class DeviceInstanceController(BaseProjectGroupController):
    """ A wrapper for DeviceInstanceModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceInstanceModel)
    active_config = Property(Instance(DeviceConfigurationModel))

    # A proxy for online and offline device configurations
    project_device = Instance(ProjectDeviceInstance, allow_none=True)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)

        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(partial(self._edit_device,
                                              parent_project))
        config_menu = self._create_sub_menu(menu, parent_project)
        dupe_action = QAction('Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_device,
                                              parent_project))
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_device,
                                                parent_project))
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
        menu.addSeparator()
        menu.addAction(instantiate_action)
        menu.addAction(shutdown_action)
        return menu

    def create_ui_data(self):
        ui_data = ProjectControllerUiData()
        self._update_icon(ui_data)
        return ui_data

    def single_click(self, parent_project, parent=None):
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

        update_check_state(self)

        return get_topology().get_project_device(
            device.instance_id, device.class_id, device.server_id,
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

    @on_trait_change("model:active_config_ref")
    def _active_config_ref_change(self):
        # Watch for incomplete model and view initialization
        if not self.model.initialized:
            return

        model = self._get_active_config()
        if model is not None:
            self.project_device.set_project_config_hash(model.configuration)

        # Ignore for online devices!
        if not self.project_device.online:
            self._broadcast_item_click()

        update_check_state(self)

    @on_trait_change('project_device:schema_updated,project_device:online')
    def _device_schema_changed(self):
        """Called when a new Schema arrives for `project_device` or it changes
        between online <-> offline.
        """
        # Show the device's configuration, iff it was already showing
        self._update_configurator()

    @on_trait_change('project_device:configuration_updated')
    def _active_config_changed_in_configurator(self):
        """Called whenever a box related to a widget is edited
        """
        configuration = self.project_device.current_configuration
        if configuration.descriptor is None:
            return

        hsh, _ = configuration.toHash()  # Ignore returned attributes
        config = self.active_config
        if config is not None:
            config.configuration = hsh

    @on_trait_change("project_device.status")
    def status_change(self):
        status_enum = DeviceStatus(self.project_device.status)
        icon = get_project_device_status_icon(status_enum)
        if icon is not None:
            self.ui_data.icon = icon
            self.ui_data.status = self.project_device.status

        # Show the device's configuration, iff it was already showing
        self._update_configurator()

    # ----------------------------------------------------------------------
    # Util methods

    def _active_config_changed(self, config_model):
        """Whenever the active config is changed from the context menu, update
        the relevant device ``Configuration`` object
        """
        if not self.model.initialized or config_model.configuration is None:
            return

        device = self.model
        configuration = self.project_device.current_configuration
        if self.project_device.online:
            configuration.dispatchUserChanges(config_model.configuration)
        else:
            configuration.fromHash(config_model.configuration)
        device.active_config_ref = config_model.uuid

    def _broadcast_item_click(self):
        configuration = self.project_device.current_configuration
        if configuration is not None:
            broadcast_event(KaraboEventSender.ShowConfiguration,
                            {'configuration': configuration})

    def _update_icon(self, ui_data):
        # Get current status of device
        if self.model.initialized:
            # But only if our model is initialized!
            status = self.project_device.status
        else:
            # Otherwise show the instance as offline
            status = 'offline'
        ui_data.icon = get_project_device_status_icon(DeviceStatus(status))
        ui_data.status = status

    def _update_configurator(self):
        configuration = self.project_device.current_configuration
        broadcast_event(KaraboEventSender.UpdateDeviceConfigurator,
                        {'configuration': configuration})

    def _create_sub_menu(self, parent_menu, parent_project):
        """ Create sub menu for parent menu and return it
        """
        config_menu = QMenu('Configuration', parent_menu)
        add_action = QAction('Add device configuration', config_menu)
        add_action.triggered.connect(partial(self._add_configuration,
                                             parent_project))
        config_menu.addAction(add_action)
        for dev_conf in self.model.configs:
            conf_action = QAction(dev_conf.simple_name, config_menu)
            conf_action.setCheckable(True)
            callback = partial(self._active_config_changed, dev_conf)
            conf_action.triggered.connect(callback)
            is_active = self.model.active_config_ref == dev_conf.uuid
            conf_action.setChecked(is_active)
            config_menu.addAction(conf_action)

        return config_menu

    # ----------------------------------------------------------------------
    # QAction handlers

    def _delete_device(self, project):
        """ Remove the device associated with this item from its device server
        """
        device = self.model
        server_model = find_parent_object(device, project,
                                          DeviceServerModel)
        if device in server_model.devices:
            server_model.devices.remove(device)

    def _edit_device(self, project):
        # Watch for incomplete model initialization
        if not self.model.initialized:
            return

        device = self.model
        server_model = find_parent_object(device, project,
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

    def _add_configuration(self, project):
        device = self.model
        server_model = find_parent_object(device, project,
                                          DeviceServerModel)
        dialog = DeviceHandleDialog(server_id=server_model.server_id,
                                    model=device, add_config=True)
        result = dialog.exec()
        if result == QDialog.Accepted:
            config_model = DeviceConfigurationModel(
                class_id=dialog.class_id, configuration=Hash(),
                simple_name=dialog.configuration_name,
                description=dialog.description
            )
            # Set initialized and modified last
            config_model.initialized = config_model.modified = True
            device.configs.append(config_model)
            device.active_config_ref = config_model.uuid

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

    def _instantiate_device(self, project):
        server = find_parent_object(self.model, project, DeviceServerModel)
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
