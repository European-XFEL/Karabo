#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import (
    Bool, DelegatesTo, HasStrictTraits, Instance, Int, Property, String,
    on_trait_change)

from karabo.middlelayer import Hash
from karabogui.binding.api import (
    BaseDeviceProxy, DeviceProxy, ProjectDeviceProxy,
    apply_configuration, apply_default_configuration, extract_configuration
)
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
    _deferred_update = Bool(False)

    # Monitor count
    _monitor_count = Int(0)

    def __init__(self, device_id, server_id, class_id):
        super(ProjectDeviceInstance, self).__init__()

        self._init_object_state(device_id, server_id, class_id)

    def get_current_config_hash(self):
        """Extract a complete device config hash from the proxy
        """
        if len(self.proxy.binding.value) > 0:
            return extract_configuration(self.proxy.binding)
        return Hash()

    def rename(self, device_id='', server_id='', class_id=''):
        """Assign a new device_id, server_id, class_id.
        """
        device_id = device_id or self.device_id
        server_id = server_id or self.server_id
        class_id = class_id or self.class_id

        # First check to see if anything is changing!
        if (device_id == self.device_id and class_id == self.class_id
                and server_id == self.server_id):
            return

        self._init_object_state(device_id, server_id, class_id)

    def set_project_config_hash(self, config):
        """Forcibly set the offline configuration Hash of the device.
        """
        if config is None:
            return

        # Make sure the offline proxy has schema first
        if len(self._offline_proxy.binding.value) > 0:
            apply_default_configuration(self._offline_proxy.binding)
            apply_configuration(config, self._offline_proxy.binding,
                                notify=False)
        else:
            self._deferred_update = True
            self._offline_config = config

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

    @on_trait_change('_offline_proxy:schema_update')
    def _apply_offline_config(self):
        """Triggered when offline proxy schema arrrived
        """
        if self._deferred_update:
            apply_default_configuration(self._offline_proxy.binding)
            apply_configuration(self._offline_config,
                                self._offline_proxy.binding, notify=False)
            self._deferred_update = False

    # ---------------------------------------------------------------------
    # utils

    def _init_object_state(self, device_id, server_id, class_id):
        """Initialize the object for a given device_id/server_id/class_id.
        """
        topology = get_topology()

        self._deferred_update = False
        if self._offline_proxy is not None:
            topology.remove_project_device_proxy(
                 self.device_id, self.server_id, self.class_id
            )

        self._online_proxy = topology.get_device(device_id)
        self._offline_proxy = topology.get_project_device_proxy(
            device_id, server_id, class_id)

        # Remember the IDs (also notifies the outside world of changes)
        self.device_id = device_id
        self.class_id = class_id
        self.server_id = server_id
