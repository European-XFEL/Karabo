#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import HasStrictTraits, Instance, Property, String, WeakRef

from karabo.middlelayer import Hash
from karabogui.binding.api import (
    BaseDeviceProxy, BindingRoot, DeviceProxy, DeviceClassProxy,
    apply_configuration, apply_default_configuration, build_binding
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
    # The proxy for this device
    proxy = Property(Instance(BaseDeviceProxy),
                     depends_on='_online_proxy.online')

    # A weak reference to the system topology device node
    device_node = WeakRef(SystemTreeNode, allow_none=True)

    # The online/offline proxies and configurations
    _online_proxy = Instance(DeviceProxy)
    _offline_proxy = Instance(DeviceClassProxy)
    _offline_config = Instance(Hash)

    def __init__(self, device_id, class_id, server_id):
        super(ProjectDeviceInstance, self).__init__()

        self._init_object_state(device_id, class_id, server_id)

    def rename(self, device_id='', class_id='', server_id=''):
        """Assign a new device_id, class_id, or server_id.
        """
        device_id = device_id or self.device_id
        class_id = class_id or self.class_id
        server_id = server_id or self.server_id

        # First check to see if anything is changing!
        if (device_id == self.device_id and class_id == self.class_id
                and server_id == self.server_id):
            return

        self._init_object_state(device_id, class_id, server_id)

    def set_project_config_hash(self, config):
        """Forcibly set the offline configuration Hash of the device.
        """
        if config is None:
            return

        apply_default_configuration(self._offline_proxy.binding)
        apply_configuration(config, self._offline_proxy.binding)
        self._offline_config = config

    # ---------------------------------------------------------------------
    # Traits Handlers

    def _get_proxy(self):
        if self._online_proxy.online:
            return self._online_proxy
        return self._offline_proxy

    # ---------------------------------------------------------------------
    # utils

    def _init_object_state(self, device_id, class_id, server_id):
        """Initialize the object for a given device_id/server_id/class_id.
        """
        topology = get_topology()
        self._online_proxy = topology.get_device(device_id)
        schema = topology.get_schema(server_id, class_id)
        binding = (BindingRoot(class_id=class_id)
                   if schema is None else build_binding(schema))
        self._offline_proxy = DeviceClassProxy(server_id=server_id,
                                               binding=binding)
        if self._offline_config is not None and schema is not None:
            apply_configuration(self._offline_config,
                                self._offline_proxy.binding)

        # Remember the IDs (also notifies the outside world of changes)
        self.device_id = device_id
        self.class_id = class_id
        self.server_id = server_id
