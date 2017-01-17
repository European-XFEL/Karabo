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
from karabo_gui.configuration import Configuration
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.events import (broadcast_event, KaraboBroadcastEvent,
                               KaraboEventSender)
from karabo_gui.indicators import DeviceStatus, get_project_device_status_icon
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.project.dialog.object_handle import ObjectDuplicateDialog
from karabo_gui.project.utils import save_object
from karabo_gui.singletons.api import get_manager, get_topology
from karabo_gui.topology import getClass, getDevice
from .bases import BaseProjectTreeItem


def destroy_device_shadow(shadow_model):
    """Destroys a DeviceInstanceModelItem by disconnecting any connected
    Qt signals.
    """
    if shadow_model.project_device is not None:
        config = shadow_model.project_device
        config.signalBoxChanged.disconnect(
            shadow_model._active_config_changed_in_configurator)

    if shadow_model._pending_descriptor_config is not None:
        conf = shadow_model._pending_descriptor_config
        conf.signalNewDescriptor.disconnect(shadow_model._new_descriptor)


class DeviceInstanceModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceInstanceModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceInstanceModel)
    active_config = Property(Instance(DeviceConfigurationModel))

    # Refers to the configuration box of type 'device'
    real_device = Instance(Configuration, allow_none=True)
    # Refers to the configuration box of type 'projectClass'
    project_device = Instance(Configuration, allow_none=True)

    # When `project_device` is first initialized, we receive a descriptor
    # asynchronously. We need to track this for Qt signal hygene
    _pending_descriptor_config = Instance(Configuration, allow_none=True)

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
        # Get current status of device
        self.model.status = _get_device_status(self.model.instance_id)
        icon = get_project_device_status_icon(DeviceStatus(self.model.status))
        item.setIcon(icon)
        item.setEditable(False)
        self.set_qt_item_text(item, self.model.instance_id)
        return item

    def single_click(self, parent_project, parent=None):
        config = self._get_active_config()
        if config is not None and config.configuration is not None:
            configuration = self._get_device_entry(parent_project, config)
            self._broadcast_item_click(configuration)

    @on_trait_change("model.modified,model.instance_id")
    def update_ui_label(self):
        """ Whenever the project is modified it should be visible to the user
        """
        if not self.is_ui_initialized():
            return
        self.set_qt_item_text(self.qt_item, self.model.instance_id)

    @on_trait_change("model.status")
    def status_change(self):
        if not self.is_ui_initialized():
            return
        status_enum = DeviceStatus(self.model.status)
        icon = get_project_device_status_icon(status_enum)
        if icon is not None:
            self.qt_item.setIcon(icon)

        self._broadcast_item_click(self._get_current_configuration())

    @on_trait_change("model.active_config_ref")
    def active_config_ref_change(self):
        if not self.is_ui_initialized():
            return
        self._broadcast_item_click(self._get_current_configuration())

    def _get_device_entry(self, project, active_config):
        """ Return the ``Configuration`` box for the device instance id
        depending on the fact whether the device is running or not

        :param project: The project this device belongs to
        :param active_config: The active device configuration

        :return The device configuration which is currently available
        """
        device = self.model
        config_changed_slot = self._active_config_changed_in_configurator
        if self.real_device is None:
            self.real_device = getDevice(device.instance_id)
        if self.project_device is None:
            self.project_device = Configuration(device.instance_id,
                                                'projectClass')
            server_model = find_parent_object(device, project,
                                              DeviceServerModel)
            self.project_device.serverId = server_model.server_id
            self.project_device.classId = active_config.class_id
            # Get class configuration
            conf = getClass(server_model.server_id, active_config.class_id)
            if conf.descriptor is None:
                # Connect to signal
                self._pending_descriptor_config = conf
                conf.signalNewDescriptor.connect(self._new_descriptor)
            else:
                self._set_descriptor(conf)
            # Track configuration changes
            self.project_device.signalBoxChanged.connect(config_changed_slot)

        return self._get_current_configuration()

    def _get_current_configuration(self):
        """ Return the current configuration depending on the status"""
        if self.real_device.isOnline():
            return self.real_device

        return self.project_device

    def _new_descriptor(self, conf):
        """The global ``Configuration`` has received a new class schema which
        needs to be set for the ``project_device`` as well
        """
        self._set_descriptor(conf)
        # Disconnect signal again
        assert self._pending_descriptor_config is not None
        self._pending_descriptor_config = None
        conf.signalNewDescriptor.disconnect(self._new_descriptor)

    def _set_descriptor(self, conf):
        if self.project_device.descriptor is not None:
            self.project_device.redummy()
        self.project_device.descriptor = conf.descriptor

        if self.real_device.descriptor is None:
            self.real_device.descriptor = conf.descriptor

        # Update active configuration hash
        self._merge_hash()

        # Notify configuration panel
        self._broadcast_show_configuration(self.project_device)

    def _merge_hash(self):
        """Merge the active configuration Hash into the existing"""
        if self.project_device.descriptor is None:
            return

        # Set default values for configuration
        self.project_device.setDefault()
        config = self._get_active_config()
        if config is not None and config.configuration is not None:
            self.project_device.fromHash(config.configuration)

    def _broadcast_item_click(self, configuration):
        if configuration is None:
            return
        # XXX the configurator expects the clicked configuration before
        # it can show the actual configuration
        data = {'configuration': configuration}
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.TreeItemSingleClick, data))

    def _broadcast_show_configuration(self, device_box):
        """ Notify configuration panel to show configuration of ``device_box``
        """
        data = {'configuration': device_box}
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.ShowConfiguration, data))

    def _get_active_config(self):
        """ Return the active device configuration object

        :return If a `DeviceConfigurationModel` for the `DeviceInstanceModel`s
        active `uuid` and `revision` can be found, otherwise a `NoneType` is
        returned
        """
        device = self.model
        uuid, revision = device.active_config_ref
        return device.select_config(uuid, revision)

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

    def _active_config_changed(self, config_model):
        """Whenever the active config is changed from the context menu, update
        the relevant device ``Configuration`` object
        """
        if config_model.configuration is None:
            return

        device = self.model
        configuration = self._get_current_configuration()
        if configuration is not None:
            configuration.fromHash(config_model.configuration)
        device.active_config_ref = (config_model.uuid, config_model.revision)

    def _active_config_changed_in_configurator(self):
        """ This slot it connected to the signal
        ``Configuration.signalBoxChanged`` which gets called whenever a box
        related to a widget is edited
        """
        hsh, _ = self.project_device.toHash()  # Ignore returned attributes
        config = self._get_active_config()
        if config is not None:
            config.configuration = hsh

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
                alias=dialog.alias, description=dialog.description,
                initialized=True
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
                                     device.instance_id,
                                     dev_conf.configuration)

    def shutdown_device(self, show_confirm=True):
        device = self.model
        get_manager().shutdownDevice(device.instance_id, show_confirm)


# ----------------------------------------------------------------------

def _get_device_status(device_id):
    topology = get_topology()
    attributes = topology.get_attributes('device.{}'.format(device_id))
    if attributes is not None:
        return attributes.get('status', 'ok')
    return 'offline'
