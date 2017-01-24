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
from karabo.middlelayer import Hash
from karabo.middlelayer_api.project.api import (read_project_model,
                                                write_project_model)
from karabo_gui.const import PROJECT_CONTROLLER_REF
from karabo_gui.events import (broadcast_event, KaraboBroadcastEvent,
                               KaraboEventSender)
from karabo_gui.indicators import DeviceStatus, get_project_device_status_icon
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.project.dialog.object_handle import ObjectDuplicateDialog
from karabo_gui.project.utils import save_object
from karabo_gui.singletons.api import get_manager, get_topology
from karabo_gui.topology.api import ProjectDeviceInstance
from .bases import BaseProjectGroupController


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
        save_action = QAction('Save', menu)
        save_action.triggered.connect(partial(save_object, self.model))
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
        item = QStandardItem()
        item.setData(weakref.ref(self), PROJECT_CONTROLLER_REF)
        # Get current status of device
        if self.model.initialized:
            # But only if our model is initialized!
            configuration = self.project_device.current_configuration
            status_enum = DeviceStatus(configuration.status)
        else:
            # Otherwise show the instance as offline
            status_enum = DeviceStatus('offline')
        icon = get_project_device_status_icon(status_enum)
        item.setIcon(icon)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)
        self.set_qt_item_text(item, self.model.instance_id)
        return item

    def single_click(self, parent_project, parent=None):
        config = self.active_config
        if config is not None and config.configuration is not None:
            self._broadcast_item_click()

    # ----------------------------------------------------------------------
    # Traits handlers

    def _get_active_config(self):
        """Traits Property getter for the active device configuration object

        :return If a `DeviceConfigurationModel` for the `DeviceInstanceModel`s
        active `uuid` and `revision` can be found, otherwise a `NoneType` is
        returned
        """
        device = self.model
        uuid, revision = device.active_config_ref
        return device.select_config(uuid, revision)

    def _project_device_default(self):
        """Traits default initializer for `project_device`.
        """
        device = self.model
        config = self.active_config

        # Complain loudly if this trait is initialized when the model isn't
        assert device.initialized, "DeviceInstanceModel must be initialized!"

        return get_topology().get_project_device(
            device.instance_id, device.class_id, device.server_id,
            None if config is None else config.configuration
        )

    @on_trait_change("model:instance_id")
    def _forced_project_device_change(self):
        """Just to make life difficult, the instance_id changed. The
        `project_device` trait needs to be reinitialized.
        """
        # Watch for incomplete model initialization
        if not self.model.initialized:
            return

        # Reuse the traits default initializer
        self.project_device = self._project_device_default()

    @on_trait_change("model.modified,model.instance_id")
    def _update_ui_label(self):
        """ Whenever the project is modified it should be visible to the user
        """
        if not self.is_ui_initialized():
            return
        self.set_qt_item_text(self.qt_item, self.model.instance_id)

    @on_trait_change("model:active_config_ref")
    def _active_config_ref_change(self):
        # Watch for incomplete model and view initialization
        if not (self.model.initialized and self.is_ui_initialized()):
            return

        model = self._get_active_config()
        if model is not None:
            self.project_device.set_project_config_hash(model.configuration)

        # Ignore for online devices!
        if not self.project_device.online:
            self._broadcast_item_click()

    @on_trait_change('project_device:schema_updated,project_device:online')
    def _device_schema_changed(self):
        """Called when a new Schema arrives for `project_device` or it changes
        between online <-> offline.
        """
        if not self.is_ui_initialized():
            return
        self._broadcast_item_click()

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

    @on_trait_change("project_device.online")
    def status_change(self):
        if not self.is_ui_initialized():
            return

        configuration = self.project_device.current_configuration
        status_enum = DeviceStatus(configuration.status)
        icon = get_project_device_status_icon(status_enum)
        if icon is not None:
            self.qt_item.setIcon(icon)

        # XXX: If we are currently showing in the configuration panel, then
        # The view there should get our now online/offline configuration

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
        configuration.fromHash(config_model.configuration)
        device.active_config_ref = (config_model.uuid, config_model.revision)

    def _broadcast_item_click(self):
        # XXX the configurator expects the clicked configuration before
        # it can show the actual configuration

        if not self.model.initialized:
            return

        configuration = self.project_device.current_configuration
        if configuration.descriptor is not None:
            data = {'configuration': configuration}
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.ShowConfiguration, data))
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.TreeItemSingleClick, data))

    def _create_sub_menu(self, parent_menu, parent_project):
        """ Create sub menu for parent menu and return it
        """
        config_menu = QMenu('Configuration', parent_menu)
        add_action = QAction('Add device configuration', config_menu)
        add_action.triggered.connect(partial(self._add_configuration,
                                             parent_project))
        config_menu.addAction(add_action)
        active_uuid, active_rev = self.model.active_config_ref
        for dev_conf in self.model.configs:
            text = '{} <{}>'.format(dev_conf.alias, dev_conf.revision)
            conf_action = QAction(text, config_menu)
            conf_action.setCheckable(True)
            callback = partial(self._active_config_changed, dev_conf)
            conf_action.triggered.connect(callback)
            dev_conf_ref = (dev_conf.uuid, dev_conf.revision)
            is_active = self.model.active_config_ref == dev_conf_ref
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
                simple_name=dialog.alias, alias=dialog.alias,
                description=dialog.description, initialized=True, modified=True
            )
            active_config_ref = (config_model.uuid, config_model.revision)
            device.configs.append(config_model)
            device.active_config_ref = active_config_ref

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
                    class_id=device.class_id,
                    instance_id=simple_name,
                    if_exists=device.if_exists,
                    active_config_ref=config_ref,
                    configs=[dupe_dev_conf],
                    initialized=True,
                    modified=True
                )
                server_model.devices.append(dev_inst)

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
                                     device.instance_id,
                                     dev_conf.configuration)

    def shutdown_device(self, show_confirm=True):
        device = self.model
        get_manager().shutdownDevice(device.instance_id, show_confirm)
