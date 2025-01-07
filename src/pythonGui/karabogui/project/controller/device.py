#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from functools import partial
from io import StringIO

from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QAction, QDialog, QMenu, QMessageBox
from traits.api import Instance, Property, on_trait_change

import karabogui.icons as icons
from karabo.common.api import Capabilities
from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    find_parent_object)
from karabo.native import Hash, read_project_model, write_project_model
from karabogui import messagebox
from karabogui.access import (
    AccessRole, access_role_allowed, get_access_level_for_role)
from karabogui.binding.api import (
    NO_CLASS_STATUSES, NO_CONFIG_STATUSES, ONLINE_CONFIG_STATUSES)
from karabogui.dialogs.api import (
    ConfigComparisonDialog, ConfigurationFromNameDialog,
    ConfigurationFromPastDialog, DeviceCapabilityDialog)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.indicators import get_project_device_status_icon
from karabogui.itemtypes import ProjectItemTypes
from karabogui.logger import get_logger
from karabogui.project.dialog.device_handle import DeviceHandleDialog
from karabogui.project.dialog.object_handle import ObjectDuplicateDialog
from karabogui.project.utils import (
    check_device_config_exists, check_device_instance_exists)
from karabogui.request import (
    get_macro_from_server, get_scene_from_server, retrieve_default_scene)
from karabogui.singletons.api import get_config, get_manager, get_topology
from karabogui.topology.api import ProjectDeviceInstance
from karabogui.util import move_to_cursor

from .bases import BaseProjectGroupController, ProjectControllerUiData
from .server import DeviceServerController

ACCESS_LEVEL_TOOLTIP = "Requires minimum '{}' access level"
SERVER_OFFLINE_TOOLTIP = "Server is offline"
DEVICE_ONLINE_TOOLTIP = "Device is already instantiated"
DEVICE_OFFLINE_TOOLTIP = "Device is offline"
INCORRECT_STATE_TOOLTIP = "Device is not in correct State"


class DeviceInstanceController(BaseProjectGroupController):
    """ A wrapper for DeviceInstanceModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceInstanceModel)
    active_config = Property(Instance(DeviceConfigurationModel))

    # A proxy for online and offline device configurations
    project_device = Instance(ProjectDeviceInstance)

    def context_menu(self, project_controller, parent=None):
        def _test_mask(mask, bit):
            return (mask & bit) == bit

        menu = QMenu(parent)
        menu.setToolTipsVisible(True)
        server_controller = find_parent_object(self, project_controller,
                                               DeviceServerController)
        server_online = server_controller.online
        proj_device_online = self.project_device.online
        proj_device_status = self.project_device.proxy.status
        proj_topo_node = self.project_device.device_node

        capabilities = proj_topo_node.capabilities if proj_topo_node else 0

        project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)
        service_allowed = access_role_allowed(AccessRole.SERVICE_EDIT)

        project_edit_access_level = get_access_level_for_role(
            AccessRole.PROJECT_EDIT)
        project_allowed_tooltip = ACCESS_LEVEL_TOOLTIP.format(
            project_edit_access_level)

        service_edit_access_level = get_access_level_for_role(
            AccessRole.SERVICE_EDIT)
        service_allowed_tooltip = ACCESS_LEVEL_TOOLTIP.format(
            service_edit_access_level)

        edit_action = QAction(icons.edit, 'Edit', menu)
        edit_action.triggered.connect(partial(self._edit_device,
                                              project_controller,
                                              parent=parent))
        edit_action.setEnabled(project_allowed)
        if not project_allowed:
            edit_action.setToolTip(project_allowed_tooltip)

        config_menu = self._create_sub_menu(menu, project_controller,
                                            project_allowed)
        dupe_action = QAction(icons.editCopy, 'Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_device,
                                              project_controller,
                                              parent=parent))
        dupe_action.setEnabled(project_allowed)
        if not project_allowed:
            dupe_action.setToolTip(project_allowed_tooltip)

        delete_action = QAction(icons.delete, 'Delete', menu)
        delete_action.triggered.connect(partial(self._delete_device,
                                                project_controller,
                                                parent=parent))
        delete_action.setEnabled(project_allowed)
        if not project_allowed:
            delete_action.setToolTip(project_allowed_tooltip)
        macro_action = QAction(icons.download, 'Open device macro', menu)
        has_macro = _test_mask(capabilities, Capabilities.PROVIDES_MACROS)
        macro_action.triggered.connect(partial(self._load_macro_from_device,
                                               project_controller,
                                               parent=parent))
        macro_action.setEnabled(has_macro and project_allowed)
        if not has_macro:
            macro_action.setToolTip("The device has no Macro")
        elif not project_allowed:
            macro_action.setToolTip(project_allowed_tooltip)

        scene_action = QAction(icons.download, 'Open device scene', menu)
        has_scene = _test_mask(capabilities, Capabilities.PROVIDES_SCENES)
        scene_action.triggered.connect(partial(self._load_scene_from_device,
                                               project_controller,
                                               parent=parent))
        scene_action.setEnabled(has_scene and project_allowed)
        if not has_scene:
            scene_action.setToolTip("Device has no scene")
        elif not project_allowed:
            scene_action.setToolTip(project_allowed_tooltip)

        conf_action = QAction(icons.clock, 'Get configuration (time)', menu)
        can_get_conf = (server_online and
                        proj_device_status not in NO_CONFIG_STATUSES)
        conf_action.triggered.connect(partial(
            self._get_configuration_from_past, parent=parent))
        conf_action.setEnabled(can_get_conf)
        if not server_online:
            conf_action.setToolTip(SERVER_OFFLINE_TOOLTIP)
        elif not can_get_conf:
            conf_action.setToolTip(INCORRECT_STATE_TOOLTIP)

        conf_action_name = QAction('Get && save configuration (name)', menu)
        can_get_conf_name = (server_online and
                             proj_device_status not in NO_CONFIG_STATUSES)
        conf_action_name.triggered.connect(partial(
            self._get_configuration_from_name, parent=parent))
        conf_action_name.setEnabled(can_get_conf_name)
        if not server_online:
            conf_action.setToolTip(SERVER_OFFLINE_TOOLTIP)
        elif not can_get_conf:
            conf_action.setToolTip(INCORRECT_STATE_TOOLTIP)

        show_action = QAction(icons.deviceInstance, 'Select in topology', menu)
        show_action.triggered.connect(self._show_device)
        show_action.setVisible(proj_device_online)

        set_conf_action = QAction('Store online configuration', menu)
        set_can_get_conf = (server_online and
                            proj_device_status in ONLINE_CONFIG_STATUSES and
                            project_allowed)
        set_conf_action.setEnabled(set_can_get_conf)
        if not server_online:
            set_conf_action.setToolTip(SERVER_OFFLINE_TOOLTIP)
        elif not project_allowed:
            set_conf_action.setToolTip(project_allowed_tooltip)
        elif not set_can_get_conf:
            set_conf_action.setToolTip(INCORRECT_STATE_TOOLTIP)
        set_conf_action.triggered.connect(partial(
            self._store_online_configuration, parent=parent))

        instantiate_action = QAction(icons.run, 'Instantiate', menu)

        can_instantiate = (server_online and not proj_device_online and
                           proj_device_status not in NO_CLASS_STATUSES)
        instantiate_action.triggered.connect(partial(self._instantiate_device,
                                                     project_controller,
                                                     parent=parent))
        instantiate_action.setEnabled(can_instantiate and service_allowed)
        if not server_online:
            instantiate_action.setToolTip(INCORRECT_STATE_TOOLTIP)
        elif proj_device_online:
            instantiate_action.setToolTip(DEVICE_ONLINE_TOOLTIP)
        elif not can_instantiate:
            instantiate_action.setToolTip(INCORRECT_STATE_TOOLTIP)
        elif not service_allowed:
            instantiate_action.setToolTip(service_allowed_tooltip)

        shutdown_action = QAction(icons.kill, 'Shutdown', menu)
        shutdown_action.triggered.connect(partial(self.shutdown_device,
                                                  show_confirm=True,
                                                  parent=parent))
        shutdown_action.setEnabled(proj_device_online and service_allowed)
        if not proj_device_online:
            shutdown_action.setToolTip(DEVICE_OFFLINE_TOOLTIP)
        elif not service_allowed:
            shutdown_action.setToolTip(service_allowed_tooltip)

        menu.addSeparator()
        about_action = QAction(icons.about, 'About', menu)
        about_action.triggered.connect(partial(self._about_device,
                                               parent=parent))

        up_action = QAction(icons.arrowFancyUp, 'Move up', menu)
        up_action.triggered.connect(partial(self._move_up,
                                            project_controller))
        up_action.setEnabled(project_allowed)
        if not project_allowed:
            up_action.setToolTip(project_allowed_tooltip)

        down_action = QAction(icons.arrowFancyDown, 'Move down', menu)
        down_action.triggered.connect(partial(self._move_down,
                                              project_controller))
        down_action.setEnabled(project_allowed)
        if not project_allowed:
            down_action.setToolTip(project_allowed_tooltip)

        menu.addAction(edit_action)
        menu.addMenu(config_menu)
        menu.addSeparator()
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(macro_action)
        menu.addAction(scene_action)
        menu.addAction(conf_action)
        menu.addAction(conf_action_name)
        menu.addAction(show_action)
        menu.addAction(set_conf_action)
        menu.addSeparator()
        menu.addAction(instantiate_action)
        menu.addAction(shutdown_action)
        menu.addSeparator()
        menu.addAction(about_action)
        menu.addAction(up_action)
        menu.addAction(down_action)

        return menu

    def info(self):
        topology_node = self.project_device.device_node
        capabilities = topology_node.capabilities if topology_node else 0

        return {'type': ProjectItemTypes.DEVICE,
                'classId': self.model.class_id,
                'deviceId': self.model.instance_id,
                'capabilities': capabilities}

    def create_ui_data(self):
        ui_data = ProjectControllerUiData()
        self._update_icon(ui_data)
        self._update_instance_status(ui_data)
        return ui_data

    def single_click(self, project_controller, parent=None):
        if not self.model.initialized:
            return

        active_config = self.active_config
        if active_config is None:
            return

        self._broadcast_item_click()

    def double_click(self, project_controller, parent=None):
        project_device = self.project_device
        if project_device is None:
            return
        retrieve_default_scene(project_device.device_id)

    def delete_press(self, project_controller, parent=None):
        """Reimplemented function on `BaseProjectController`"""
        self._delete_device(project_controller, parent)

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
            None if config is None else config.configuration)

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
            class_id=self.model.class_id)

    @on_trait_change("model:initialized,model:instance_id")
    def _update_ui_label(self):
        """ Whenever the object is modified it should be visible to the user
        """
        self._update_icon(self.ui_data)
        self._update_instance_status(self.ui_data)

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

    @on_trait_change('project_device:save_project')
    def _active_config_changed_in_configurator(self):
        """Called whenever project device's offline configuration is changed
        """
        if self.project_device.online:
            return
        config = self.active_config
        if config is not None:
            new_config = self.project_device.get_user_edited_config_hash()
            config.configuration = new_config

    @on_trait_change("project_device:status")
    def status_change(self, status):
        self._update_icon(self.ui_data)
        # Show the device's configuration, iff it was already showing
        self._update_configurator()

    @on_trait_change("project_device:instance_status")
    def instance_status_change(self, status):
        self._update_instance_status(self.ui_data)

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
        """Broadcast the request to view the project device

        This is not a lazy action. The device can be either online or offline
        and must be shown with a schema.
        """
        proxy = self.project_device.proxy
        if not self.project_device.online:
            proxy.ensure_class_schema()

        broadcast_event(KaraboEvent.ShowConfiguration, {'proxy': proxy})

    def _update_icon(self, ui_data):
        # Get current status of device
        if not self.model.initialized:
            return

        status_enum = self.project_device.status
        ui_data.icon = get_project_device_status_icon(status_enum)
        ui_data.status = status_enum

    def _update_instance_status(self, ui_data):
        if not self.model.initialized:
            return
        ui_data.instance_status = self.project_device.instance_status

    def _update_configurator(self):
        broadcast_event(KaraboEvent.UpdateDeviceConfigurator,
                        {'proxy': self.project_device.proxy})

    def _create_sub_menu(self, parent_menu, project_controller, allowed):
        """ Create sub menu for parent menu and return it
        """
        config_menu = QMenu('Configuration', parent_menu)
        add_action = QAction('Add device configuration', config_menu)
        add_action.triggered.connect(partial(self._add_configuration,
                                             project_controller,
                                             parent=parent_menu.parent()))
        add_action.setEnabled(allowed)

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

    def _show_device(self):
        deviceId = self.model.instance_id
        broadcast_event(KaraboEvent.ShowDevice,
                        {'deviceId': deviceId, 'showTopology': True})

    def _delete_device(self, project_controller, parent=None):
        """ Remove the device associated with this item from its device server
        """
        device = self.model
        development = get_config()["development"]
        if not development:
            ask = ('Are you sure you want to delete \"<b>{}</b>\".<br /> '
                   'Continue action?'.format(device.instance_id))
            msg_box = QMessageBox(QMessageBox.Question, 'Delete device',
                                  ask, QMessageBox.Yes | QMessageBox.No,
                                  parent=parent)
            msg_box.setModal(False)
            msg_box.setDefaultButton(QMessageBox.No)
            move_to_cursor(msg_box)
            if msg_box.exec() != QMessageBox.Yes:
                return

        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)
        if device in server_model.devices:
            server_model.devices.remove(device)

    def _edit_device(self, project_controller, parent=None):
        # Watch for incomplete model initialization
        if not self.model.initialized:
            return

        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)
        dialog = DeviceHandleDialog(server_id=server_model.server_id,
                                    model=device,
                                    is_online=self.project_device.online,
                                    parent=parent)
        move_to_cursor(dialog)
        result = dialog.exec()
        if result == QDialog.Accepted:
            # Check for existing device
            renamed = device.instance_id != dialog.instance_id
            if renamed and check_device_instance_exists(dialog.instance_id):
                return

            # We might create a new project device here when renaming!
            device.instance_id = dialog.instance_id
            # Look for existing DeviceConfigurationModel
            conf_model = device.select_config(dialog.active_uuid)
            if conf_model is not None:
                device.active_config_ref = dialog.active_uuid
                conf_model.class_id = dialog.class_id
                conf_model.description = dialog.description
                # When renaming, we have to apply the config hash!
                if renamed:
                    project_dev = self.project_device
                    proxy = project_dev.proxy
                    config_hash = conf_model.configuration
                    project_dev.set_project_config_hash(config_hash)
                    broadcast_event(KaraboEvent.UpdateDeviceConfigurator,
                                    {"proxy": proxy, "refresh": True})

    def _about_device(self, parent=None):
        device = self.model
        info = {}
        info["deviceId"] = device.instance_id
        info["classId"] = device.class_id
        info["serverId"] = device.server_id
        info["uuid"] = device.uuid

        htmlString = ("<table>" +
                      "".join("<tr><td><b>{}</b>:   </td><td>{}</td></tr>".
                              format(*p) for p in info.items()) + "</table>")
        messagebox.show_information(htmlString, parent=parent)

    def _move_up(self, project_controller):
        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)

        if device in server_model.devices:
            index = server_model.devices.index(device)
            new_index = index - 1
            if new_index < 0:
                return
            devices = list(server_model.devices)
            del server_model.devices[:]
            devices.remove(device)
            devices.insert(new_index, device)
            server_model.devices.extend(devices)

    def _move_down(self, project_controller):
        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)

        if device in server_model.devices:
            index = server_model.devices.index(device)
            new_index = index + 1
            if new_index > len(server_model.devices):
                return
            devices = list(server_model.devices)
            del server_model.devices[:]
            devices.remove(device)
            devices.insert(new_index, device)
            server_model.devices.extend(devices)

    def _add_configuration(self, project_controller, parent=None):
        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)
        dialog = DeviceHandleDialog(server_id=server_model.server_id,
                                    model=device, add_config=True,
                                    parent=parent)
        move_to_cursor(dialog)
        result = dialog.exec()
        if result == QDialog.Accepted:
            # Check for existing device configuration
            if check_device_config_exists(device.instance_id,
                                          dialog.configuration_name):
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

    def _duplicate_device(self, project_controller, parent=None):
        """ Duplicate the active device configuration of the model

        :param project: The parent project controller the model belongs to
        """
        device = self.model
        server_model = find_parent_object(device, project_controller.model,
                                          DeviceServerModel)
        active_config = self.active_config
        if active_config is None:
            return

        dialog = ObjectDuplicateDialog(device.instance_id, parent=parent)
        move_to_cursor(dialog)
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
                    active_config_ref=dupe_dev_conf.uuid,
                    configs=[dupe_dev_conf]
                )
                # Set initialized and modified last
                dev_inst.initialized = dev_inst.modified = True
                server_model.devices.append(dev_inst)

    def _instantiate_device(self, project_controller, parent=None):
        server = find_parent_object(self.model, project_controller.model,
                                    DeviceServerModel)
        self.instantiate(server, parent)

    def _load_macro_from_device(self, project_controller, parent=None):
        """Request a scene directly from a device
        """
        dialog = DeviceCapabilityDialog(
            device_id=self.model.instance_id,
            capability=Capabilities.PROVIDES_MACROS,
            parent=parent)
        if dialog.exec() == QDialog.Accepted:
            device_id = dialog.device_id
            macro_name = dialog.capa_name
            project = project_controller.model
            project_macros = {s.simple_name for s in project.macros}

            if f'{device_id}-{macro_name}' in project_macros:
                msg = ('A macro with that name already exists in the selected '
                       'project.')
                messagebox.show_warning(msg, title='Cannot Load Macro')
                return

            get_macro_from_server(device_id, macro_name=macro_name,
                                  project=project)

    def _load_scene_from_device(self, project_controller, parent=None):
        """Request a scene directly from a device
        """
        dialog = DeviceCapabilityDialog(device_id=self.model.instance_id,
                                        parent=parent)
        if dialog.exec() == QDialog.Accepted:
            device_id = dialog.device_id
            scene_name = dialog.capa_name
            project = project_controller.model
            project_scenes = {s.simple_name for s in project.scenes}

            if f'{device_id}|{scene_name}' in project_scenes:
                msg = ('A scene with that name already exists in the selected '
                       'project.')
                messagebox.show_warning(msg, title='Cannot Load Scene')
                return

            get_scene_from_server(device_id, scene_name=scene_name,
                                  project=project)

    def _get_configuration_from_past(self, parent=None):
        """Request a configuration from the datalog reader
        """
        dialog = ConfigurationFromPastDialog(
            instance_id=self.model.instance_id, parent=parent)
        dialog.move(QCursor.pos())
        dialog.show()
        dialog.raise_()
        dialog.activateWindow()

    def _get_configuration_from_name(self, parent=None):
        """Request a configuration from name from configuration manager"""
        device_id = self.model.instance_id
        dialog = ConfigurationFromNameDialog(
            instance_id=device_id, parent=parent)
        dialog.move(QCursor.pos())
        dialog.show()
        dialog.raise_()
        dialog.activateWindow()

    def _store_online_configuration(self, parent=None):
        """Set the online to offline configuration of this project device"""
        if not self.project_device.online:
            return

        device = self.project_device

        def _set_device_configuration():
            success, config = device.get_user_edited_config_hash_online()
            if success:
                title = f"Device configuration for {device.device_id}"
                old = self.active_config.configuration
                dialog = ConfigComparisonDialog(title, old, config,
                                                parent=parent)
                if dialog.exec() == QDialog.Accepted:
                    # Note: Make sure the active config is stored and
                    # apply the configuration to the online system!
                    self.active_config.configuration = config
                    self.project_device.set_project_config_hash_online(config)
            else:
                messagebox.show_error(
                    "The device has elements of type <b>ChoiceOfNodes</b>, "
                    "<b>ListOfNodes</b> or <b>DeviceNode</b> in the existing "
                    "<b>Schema</b>. Online to offline configuration is "
                    "forbidden.", parent=parent)

        # Note: The device is online, but we require an offline schema
        # to compare for online to offline config extraction
        proxy = device._offline_proxy
        has_offline = len(proxy.binding.value) > 0
        if has_offline:
            _set_device_configuration()
        else:

            def _schema_handler():
                proxy.on_trait_change(_schema_handler, "schema_update",
                                      remove=True)
                _set_device_configuration()

            # There must be a class schema, since we are online. Hence,
            # we can safely wait here for the class schema of the server!
            proxy.on_trait_change(_schema_handler, "schema_update")
            proxy.ensure_class_schema()

    def instantiate(self, server, parent=None):
        """ Instantiate this device instance on the given `server`

        :param server: The server this device belongs to
        """
        if self.project_device.online:
            return

        device = self.project_device
        classId = device.class_id
        serverId = device.server_id
        if device.status in NO_CLASS_STATUSES:
            msg = (f"Unknown class schema for device plugin {classId} on "
                   f"server {serverId}. Please make sure to install the "
                   "device plugin first.")
            get_logger().warning(msg)
            return

        proxy = device.proxy
        deviceId = proxy.device_id
        get_manager().initDevice(serverId, classId, deviceId)

    def shutdown_device(self, show_confirm=True, parent=None):
        device = self.model
        get_manager().shutdownDevice(device.instance_id, show_confirm, parent)
