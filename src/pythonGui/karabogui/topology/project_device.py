#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import (
    Bool, DelegatesTo, HasStrictTraits, Instance, on_trait_change, Property,
    String, WeakRef)

from karabo.middlelayer import Hash
from karabogui.binding.api import (
    BaseDeviceProxy, DeviceProxy, ProjectDeviceProxy,
    apply_configuration, apply_default_configuration, extract_configuration
)
from karabogui.singletons.api import get_topology
from .tree import SystemTreeNode


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
    device_node = WeakRef(SystemTreeNode, allow_none=True)

    # The proxy for this device
    proxy = Property(Instance(BaseDeviceProxy),
                     depends_on='_online_proxy.online')
    online = DelegatesTo('_online_proxy')

    # The online/offline proxies and configurations
    _online_proxy = Instance(DeviceProxy)
    _offline_proxy = Instance(ProjectDeviceProxy)

    # Internal storage for applying offline configuration
    _offline_config = Instance(Hash)
    _deferred_update = Bool(False)

    def __init__(self, device_id, server_id, class_id):
        super(ProjectDeviceInstance, self).__init__()

        self._init_object_state(device_id, server_id, class_id)

    def get_current_config_hash(self):
        """Extract a complete device config hash from the proxy
        """
        if len(self.proxy.binding.value) > 0:
            return extract_configuration(self.proxy.binding,
                                         modified_only=False)
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
            apply_configuration(config, self._offline_proxy.binding)
        else:
            self._deferred_update = True
            self._offline_config = config

    # ---------------------------------------------------------------------
    # Traits Handlers

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
                                self._offline_proxy.binding)
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
