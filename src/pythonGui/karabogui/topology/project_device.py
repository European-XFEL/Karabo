#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import (
    Bool, DelegatesTo, HasStrictTraits, Instance, Int, Property, String,
    on_trait_change)

from karabo.common.api import ProxyStatus, KARABO_SCHEMA_DEFAULT_VALUE
from karabo.native import Hash
from karabogui.binding.api import (
    BaseDeviceProxy, DeviceProxy, ProjectDeviceProxy,
    apply_default_configuration, apply_project_configuration, extract_edits)
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
                     depends_on='_online_proxy.online')
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

    # Error flag of the online device proxy
    error = Bool(False)
    # The status of this device, take into consider the error flag and
    # offline proxy's status (server may be gone, status has to change
    # accordingly)
    status = Property(depends_on=['_online_proxy.online', 'error',
                                  '_offline_proxy.status'])

    def __init__(self, device_id, server_id, class_id):
        super(ProjectDeviceInstance, self).__init__()

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
        """Assign a new device_id, server_id, class_id.
        """
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
        if self._online_proxy.online:
            status = self._online_proxy.status
            if self.error:
                status = ProxyStatus.ERROR
            elif self.class_id != self._online_proxy.binding.class_id:
                status = ProxyStatus.INCOMPATIBLE
        else:
            status = self._offline_proxy.status

        return status

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
                text = (f"Apply offline configuration for <b>{device_id}</b> "
                        f"reported <b>{len(fails)}</b> problem(s)")
                get_logger().error(text)

    @on_trait_change('_offline_proxy.status', post_init=True)
    def _status_changed(self, old, new):
        """Clear the offline proxy binding values when device server goes
        offline
        """
        if new == ProxyStatus.NOSERVER:
            self._offline_proxy.binding.value.clear_namespace()

    # ---------------------------------------------------------------------
    # utils

    def _init_object_state(self, device_id, server_id, class_id):
        """Initialize the object for a given device_id/server_id/class_id.
        """
        topology = get_topology()

        if self._offline_proxy is not None:
            topology.remove_project_device_proxy(
                self.device_id, self.server_id, self.class_id
            )

        self._online_proxy = topology.get_device(device_id)
        self._offline_proxy = topology.get_project_device_proxy(
            device_id, server_id, class_id)

        # Help out, if the online proxy doesn't have its binding yet
        if self._online_proxy.binding.class_id == '':
            self._online_proxy.binding.class_id = class_id

        # Remember the IDs (also notifies the outside world of changes)
        self.device_id = device_id
        self.class_id = class_id
        self.server_id = server_id
