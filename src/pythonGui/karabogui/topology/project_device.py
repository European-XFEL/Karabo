#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
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

from traits.api import (
    DelegatesTo, HasStrictTraits, Instance, Int, Property, String,
    on_trait_change)

from karabo.common.api import KARABO_SCHEMA_DEFAULT_VALUE, InstanceStatus
from karabo.native import Hash
from karabogui.binding.api import (
    BaseDeviceProxy, DeviceProxy, ProjectDeviceProxy, ProxyStatus,
    apply_default_configuration, apply_project_configuration, extract_edits,
    extract_online_edits)
from karabogui.logger import get_logger
from karabogui.singletons.api import get_topology


class ProjectDeviceInstance(HasStrictTraits):
    """An abstraction of devices needed for projects and workflow design.
    No matter the online/offline state of a device, you can get a
    ``Configuration`` object which is valid and associated with the same
    device id.
    """
    # The current device id. Can be monitored for changes.
    device_id = String
    class_id = String
    server_id = String

    # A weak reference to the system topology device node
    device_node = DelegatesTo('_online_proxy', prefix='topology_node')

    # The proxy for this device
    proxy = Property(Instance(BaseDeviceProxy),
                     depends_on='_online_proxy:online')
    online = DelegatesTo('_online_proxy')
    schema_update = DelegatesTo('_online_proxy')

    # The online/offline proxies and configurations
    _online_proxy = Instance(DeviceProxy)
    _offline_proxy = Instance(ProjectDeviceProxy)

    # Internal storage for applying offline configuration
    _offline_config = Instance(Hash)

    # An event indicates there are changes in offline configurations so
    # the project should prompt for save
    save_project = DelegatesTo('_offline_proxy',
                               prefix='binding.config_update')

    # Monitor count
    _monitor_count = Int(0)

    # The proxy status
    status = Property(depends_on=['_online_proxy:online',
                                  '_offline_proxy:status'])

    # The instance status deliverd by the instance info
    instance_status = Property(
        depends_on=['_online_proxy:topology_node.status'])

    configuration = Property

    def __init__(self, device_id, server_id, class_id):
        super().__init__()
        self._init_object_state(device_id, server_id, class_id)

    def get_user_edited_config_hash(self):
        """Extract user edited values and attributes from the offline proxy
        """
        topology = get_topology()
        # To extract user edits we need a device schema to find default
        # attribute values
        schema = topology.get_schema(self.server_id, self.class_id)

        if schema is not None and len(self._offline_proxy.binding.value) > 0:
            new_config = extract_edits(schema, self._offline_proxy.binding)
            # update our own copy of the device config
            self._offline_config = new_config
            return new_config
        return Hash()

    def get_user_edited_config_hash_online(self):
        """Extract user edited values from the online proxy

        This method requires that the proxy has both an offline and online
        schema.

        returns: success boolean and configuration Hash

                 For more information see `extract_online_edits` of the config
                 module.
        """
        has_offline = len(self._offline_proxy.binding.value) > 0
        has_online = len(self._online_proxy.binding.value) > 0
        if has_offline and has_online:
            topology = get_topology()
            schema = topology.get_schema(self.server_id, self.class_id)
            config = extract_online_edits(
                schema, self._online_proxy.binding)
            return True, config

        return False, Hash()

    def get_class_proxy(self):
        """Retrieve a class schema for this project device instance"""
        topology = get_topology()
        return topology.get_class(self.server_id, self.class_id)

    def get_schema_default_configuration(self):
        """Extract the ``default`` configuration derived from a schema

        The returned configuration Hash consists of all keys which have
        with defaultValue as their value.
        """
        topology = get_topology()
        schema = topology.get_schema(self.server_id, self.class_id)
        config = Hash()
        if schema is not None:
            for key, _, attrs in schema.hash.iterall():
                default = attrs.get(KARABO_SCHEMA_DEFAULT_VALUE)
                if default is not None:
                    config[key] = default

        return config

    def rename(self, device_id='', server_id='', class_id=''):
        """Assign a new device_id, server_id, class_id."""
        device_id = device_id or self.device_id
        server_id = server_id or self.server_id
        class_id = class_id or self.class_id

        # First check to see if anything is changing!
        if (device_id == self.device_id and class_id == self.class_id and
                server_id == self.server_id):
            return

        self._init_object_state(device_id, server_id, class_id)

    def set_project_config_hash(self, config):
        """Forcibly set the offline configuration Hash of the device.
        """
        if config is None:
            return
        # Store the offline config, whenever the device goes from online to
        # offline or its schema changes, we need to apply the config again.
        self._offline_config = config

        # Only apply offline device configuration if the device is offline
        # and we have schema
        if not self.online and len(self._offline_proxy.binding.value) > 0:
            apply_default_configuration(self._offline_proxy.binding)
            # Do not report here...
            apply_project_configuration(config, self._offline_proxy.binding)

    def set_project_config_hash_online(self, config):
        if config is None or not self.online:
            return

        self._offline_config = config
        apply_default_configuration(self._offline_proxy.binding)
        apply_project_configuration(config, self._offline_proxy.binding)

    def start_monitoring(self):
        """Enable monitoring of the online device (when it is online).
        """
        self._monitor_count += 1

    def stop_monitoring(self):
        """Disable monitoring of the online device (when it is online).
        """
        self._monitor_count -= 1

    # ---------------------------------------------------------------------
    # Traits Handlers

    def _get_instance_status(self):
        """Return the online topology information"""
        node = self.device_node
        if node is not None:
            return node.status
        # None status is offline
        return InstanceStatus.NONE

    def _get_configuration(self):
        """Return the configured configuration (`offline`) for this device"""
        return self._offline_config

    def __online_proxy_changed(self, old, new):
        """Keep _monitor_count in sync with _online_proxy."""
        if old is not None and self._monitor_count > 0:
            old.remove_monitor()
        if new is not None and self._monitor_count > 0:
            new.add_monitor()

    def __monitor_count_changed(self, old, new):
        """Handle _monitor_count state transitions"""
        if old == 0 and new == 1:
            self._online_proxy.add_monitor()
        elif old == 1 and new == 0:
            self._online_proxy.remove_monitor()

    def _get_proxy(self):
        if self._online_proxy.online:
            return self._online_proxy
        return self._offline_proxy

    def _get_status(self):
        """Return the proxy status information"""
        if self._online_proxy.online:
            if self.class_id != self._online_proxy.binding.class_id:
                return ProxyStatus.INCOMPATIBLE
            return self._online_proxy.status
        return self._offline_proxy.status

    @on_trait_change('_online_proxy:online,_offline_proxy:schema_update')
    def _apply_offline_config(self):
        """Apply offline device configuration, but only is there is a schema
        """
        if not self.online and len(self._offline_proxy.binding.value) > 0:
            apply_default_configuration(self._offline_proxy.binding)
            device_id = self._offline_proxy.device_id
            if self._offline_config is None:
                text = ("Ignoring corrupted project configuration "
                        "for device {}!".format(device_id))
                get_logger().error(text)
                return

            fails = apply_project_configuration(
                self._offline_config, self._offline_proxy.binding)
            if fails:
                number = len(fails.paths(intermediate=False))
                text = (f"Apply offline configuration for <b>{device_id}</b> "
                        f"reported <b>{number}</b> problem(s)")
                get_logger().error(text)

            # Notify configuration has been applied!
            self._offline_proxy.config_update = True

    @on_trait_change('_offline_proxy:status', post_init=True)
    def _status_changed(self, old, new):
        """Clear the offline proxy binding values when device server goes
        offline
        """
        if new == ProxyStatus.NOSERVER:
            self._offline_proxy.binding.value.clear_namespace()

    # ---------------------------------------------------------------------
    # utils

    def _init_object_state(self, device_id, server_id, class_id):
        """Initialize the object for a given device_id/server_id/class_id"""
        topology = get_topology()

        if self._offline_proxy is not None:
            topology.remove_project_device_proxy(
                self.device_id, self.server_id, self.class_id)

        self._online_proxy = topology.get_device(device_id, request=False)
        self._offline_proxy = topology.get_project_device_proxy(
            device_id, server_id, class_id)

        # Note: Before we were overwriting an empty classId of the online
        # proxy if needed

        # Remember the IDs (also notifies the outside world of changes)
        self.device_id = device_id
        self.class_id = class_id
        self.server_id = server_id
