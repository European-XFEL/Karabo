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
from collections import Counter

from traits.api import (
    Any, Bool, DelegatesTo, Enum, Event, HasStrictTraits, Instance, Int,
    Property, String, WeakRef, on_trait_change)

from karabo.common import const
from karabo.native import Hash
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.api import get_network, get_topology

from .binding_types import (
    BaseBinding, BindingRoot, FloatBinding, PipelineOutputBinding, SlotBinding,
    VectorBinding)
from .enums import ONLINE_STATUSES, SCHEMA_STATUSES, ProxyStatus

_NOCAST_BINDINGS = (FloatBinding, VectorBinding)


class BaseDeviceProxy(HasStrictTraits):
    """Base class for proxying device classes, device instances, or project
    devices.
    """
    # The root of the data binding
    binding = Instance(BindingRoot)
    # The root's `state` property, if existent
    state_binding = Instance(BaseBinding)
    # The root's `lockedBy` property, if existent
    locked_binding = Instance(BaseBinding)
    # ID of the server hosting this class
    server_id = String
    # True when the device is online
    online = Property(Bool, depends_on='status')
    # The current status
    status = Enum(*ProxyStatus)
    # An event which fires when the schema is updated or otherwise changes
    schema_update = DelegatesTo('binding')

    @on_trait_change('binding.schema_update')
    def _binding_changed(self):
        """When `binding` is assigned to or its schema changes, refresh
        `state_binding`
        """
        self.state_binding = self.get_property_binding('state')
        self.locked_binding = self.get_property_binding('lockedBy')

    def _get_online(self):
        return self.status in ONLINE_STATUSES

    def _status_default(self):
        return ProxyStatus.OFFLINE

    def get_property_binding(self, path):
        """Return the ``BaseBinding``-derived instance for the proxy property
        denoted by `path`.

        NOTE: Returns `None` if the binding is not set, or the property does
        not yet exist in the binding (eg: when awaiting schema injection)
        """
        prop = self.binding
        for part in path.split('.'):
            if prop is None:
                break
            prop = getattr(prop.value, part, None)
        return prop

    def __getitem__(self, path):
        return self.get_property_binding(path)


class DeviceProxy(BaseDeviceProxy):
    """The proxy for device instances and project devices. Instances may come
    on and offline.
    """
    # ID of the device we are proxying
    device_id = String
    # An event which fires when the configuration is updated from the network
    config_update = Event
    # The system tree node for this device
    topology_node = WeakRef('karabogui.topology.system_tree.SystemTreeNode',
                            allow_none=True)

    # A reference counter for tracking the monitoring
    _monitor_count = Int(0)
    # A counter for pipeline connection objects
    _pipeline_subscriptions = Instance(Counter, args=())

    # -----------------------------------------------------------------------
    # Traits methods

    def _config_update_fired(self):
        if self.status is ProxyStatus.SCHEMA:
            self.status = ProxyStatus.ALIVE
        if self.status is ProxyStatus.ALIVE and self._monitor_count > 0:
            self.status = ProxyStatus.MONITORING

    def _schema_update_fired(self):
        if self.status is ProxyStatus.ONLINEREQUESTED:
            if self._monitor_count > 0:
                self._start_monitoring()
                # reestablish the pipelining
                for path in self._pipeline_subscriptions:
                    get_network().onSubscribeToOutput(self.device_id, path,
                                                      True)
            self.status = ProxyStatus.SCHEMA
        elif self.status in (ProxyStatus.ALIVE, ProxyStatus.MONITORING):
            get_network().onGetDeviceConfiguration(self.device_id)

    def _status_changed(self, new):
        if new is ProxyStatus.ONLINE and self._monitor_count > 0:
            self.refresh_schema()

    def __monitor_count_changed(self, old, new):
        if self.topology_node is not None:
            self.topology_node.monitoring = (new > 0)

        if old == 0 and new == 1:
            broadcast_event(KaraboEvent.StartMonitoringDevice,
                            {'device_id': self.device_id})
        elif old == 1 and new == 0:
            broadcast_event(KaraboEvent.StopMonitoringDevice,
                            {'device_id': self.device_id})

    def _topology_node_changed(self, new):
        if new is not None:
            new.monitoring = (self._monitor_count > 0)

    # -----------------------------------------------------------------------
    # Public interface

    def add_monitor(self):
        """Ask the GUI server to begin monitoring this device
        """
        ignored_statuses = (ProxyStatus.OFFLINE,
                            ProxyStatus.ONLINEREQUESTED)

        self._monitor_count += 1
        if self._monitor_count == 1:
            if self.status is ProxyStatus.ONLINE:
                self.refresh_schema()
            elif self.status not in ignored_statuses:
                self._start_monitoring()

    def remove_monitor(self):
        """Ask the GUI server to stop monitoring this device
        """
        assert self._monitor_count > 0
        self._monitor_count -= 1
        if self._monitor_count == 0:
            self._stop_monitoring()

    def connect_pipeline(self, path):
        """Ask the GUI server to subscribe to a pipeline output
        """
        if path not in self._pipeline_subscriptions:
            get_network().onSubscribeToOutput(self.device_id, path, True)

        # NOTE: We fill a counter object with the path, as we might be
        # interested in multiple values from an output channel
        self._pipeline_subscriptions[path] += 1

    def disconnect_pipeline(self, path):
        """Ask the GUI server to unsubscribe from a pipeline output
        """
        assert path in self._pipeline_subscriptions

        self._pipeline_subscriptions[path] -= 1
        # NOTE: Only if we fully removed all interested properties
        # we unsubscribe from the output channel
        if self._pipeline_subscriptions[path] == 0:
            del self._pipeline_subscriptions[path]
            get_network().onSubscribeToOutput(self.device_id, path, False)

    def publish_historic_data(self, path, data):
        """A callback which routes historic data received from the GUI server
        to the appropriate binding object.
        """
        binding = self.get_property_binding(path)
        if binding is not None:
            # NOTE: `historic_data` is an Event trait which does not store
            # any values assigned to it! The value is only available to
            # traits notification handlers which are listening to the event.
            binding.historic_data = data

    def refresh_schema(self):
        """Request a recent schema for this device instance
        """
        if self.status is not ProxyStatus.ONLINEREQUESTED:
            get_network().onGetDeviceSchema(self.device_id)
            self.status = ProxyStatus.ONLINEREQUESTED

    # -----------------------------------------------------------------------
    # Private interface

    def _start_monitoring(self):
        get_network().onStartMonitoringDevice(self.device_id)

    def _stop_monitoring(self):
        """Stop monitoring of the device proxy"""
        get_network().onStopMonitoringDevice(self.device_id)
        if self.status in (ProxyStatus.ALIVE, ProxyStatus.MONITORING):
            self.status = ProxyStatus.ONLINE

    def __repr__(self):
        return f"<{self.__class__.__name__} deviceId={self.device_id}>"


class DeviceClassProxy(BaseDeviceProxy):
    """The proxy for device classes. Used for editing before instantiation.
    """
    # An event which fires when the configuration has been applied
    config_update = Event

    def _status_default(self):
        topology = get_topology()
        server_key = f'server.{self.server_id}'
        attributes = topology.get_attributes(server_key)
        if attributes is None:
            return ProxyStatus.NOSERVER
        elif self.binding.class_id not in attributes.get('deviceClasses', []):
            return ProxyStatus.NOPLUGIN
        return ProxyStatus.OFFLINE

    def _schema_update_fired(self):
        if self.status is ProxyStatus.REQUESTED:
            self.status = self._status_default()

    def refresh_schema(self):
        """Request a recent schema for this device class
        """
        if self.status is not ProxyStatus.REQUESTED:
            get_network().onGetClassSchema(self.server_id,
                                           self.binding.class_id)
            self.status = ProxyStatus.REQUESTED

    def __repr__(self):
        return (f"<{self.__class__.__name__} "
                f"classId={getattr(self.binding, 'class_id', None)} "
                f"serverId={self.server_id}>")


class ProjectDeviceProxy(DeviceClassProxy):
    """A device class proxy used in ProjectDeviceInstance
    """
    device_id = String

    def update_status(self):
        """Request a recalculating of the device status
        """
        self.status = self._status_default()

    def ensure_class_schema(self):
        """Ensure the class schema of the project device proxy"""
        topology = get_topology()
        topology.ensure_proxy_class_schema(
            self.device_id, self.server_id, self.binding.class_id)

    def __repr__(self):
        return f"<{self.__class__.__name__} deviceId={self.device_id}>"


class PropertyProxy(HasStrictTraits):
    """A proxy for a single device property

    :param path: Full path of the property
    :param key: Full 'key' of the property: <device ID>.<path>
    :param binding: The binding for the property
    :param value: The value for the property (from the binding instance)

    :param edit_value: A user-entered value
    :param root_proxy: Parent device proxy
    :param pipeline_parent_path: Potential parent path if `binding` is a
                                 child of a Pipeline Output

    """
    path = String
    key = Property(String)
    binding = Instance(BaseBinding)
    value = Property(depends_on='binding.value')
    edit_value = Any
    root_proxy = Instance(BaseDeviceProxy, allow_none=False)
    pipeline_parent_path = String

    # More private members
    # ---------------------

    visible = Bool(False)
    # Check whether we are existing or not
    existing = Bool(True)

    # An extra binding instance for `edit_value` validation
    _edit_binding = Instance(BaseBinding)

    # -----------------------------------------------------------------------
    # Traits methods

    def _get_key(self):
        if not isinstance(self.root_proxy, DeviceProxy):
            return self.path
        return self.root_proxy.device_id + '.' + self.path

    def _get_value(self):
        binding = self.binding
        if binding is not None:
            return binding.value

    def _set_value(self, value):
        binding = self.binding
        if binding is not None:
            binding.value = value

    def _pipeline_parent_path_default(self):
        if self.binding is None:
            return ''

        def _gen_parents(p):
            if '.' in p:
                p = p.rsplit('.', 1)[0]
                yield p
                yield from _gen_parents(p)

        # Search the parents of this node (via path manipulation) for pipeline
        # outputs. Return the path of that parent if found.
        for path in _gen_parents(self.path):
            binding = self.root_proxy.get_property_binding(path)
            if isinstance(binding, PipelineOutputBinding):
                return path
        return ''  # No parent pipeline output node found

    def _pipeline_parent_path_changed(self, new):
        """Make sure to connect a pipeline if a schema arrives after we start
        monitoring!
        """
        if new == '':
            return

        if self.visible and isinstance(self.root_proxy, DeviceProxy):
            # Call blindling, the device proxy will keep things orderly
            self.root_proxy.connect_pipeline(new)

    @on_trait_change('root_proxy.schema_update,path')
    def _binding_update(self):
        # Clear some edit related traits no matter what
        self._edit_binding = None
        if self.root_proxy is None:
            return

        self.binding = self.root_proxy.get_property_binding(self.path)
        if self.root_proxy.status in SCHEMA_STATUSES:
            self.existing = self.binding is not None
            self.pipeline_parent_path = self._pipeline_parent_path_default()

    def _edit_value_changed(self, value):
        """When `edit_value` changes, set it to a binding object to validate
        its value.
        """
        if value is None:
            return

        if self._edit_binding is None:
            klass = type(self.binding)
            self._edit_binding = klass()

        # Validate (and mutate in some cases, like arrays)
        self._edit_binding.value = value
        # Replace quietly but don't send no cast bindings to gui
        self.trait_setq(edit_value=self._edit_binding.value)
        if not isinstance(self.binding, _NOCAST_BINDINGS):
            self.binding.config_update = True

    # -----------------------------------------------------------------------
    # Public methods

    def execute(self):
        """Call a slot on a remote device.
        """
        if not (isinstance(self.root_proxy, DeviceProxy) and
                isinstance(self.binding, SlotBinding)):
            return

        state = self.root_proxy.state_binding.value
        if self.binding.is_allowed(state):
            # Macro Slots are not guaranteed to reply immediately
            # we do not expect a reply from them.
            # XXX: this protection should be implemented on macro side
            t_node = self.root_proxy.topology_node
            ignore_timeouts = False
            if t_node:
                # ignore timeouts if the root_proxy is a macro.
                ignore_timeouts = t_node.attributes.get('type') == 'macro'
            network = get_network()
            network.onExecute(self.root_proxy.device_id,
                              self.path, ignore_timeouts)

            info = Hash("type", "execute",
                        "instanceId", self.root_proxy.device_id,
                        "slot", self.path)
            network.onInfo(info)

    def get_device_value(self):
        """Return the value stored in the device configuration, rather
        than that which is stored in `binding`.

        For a device class, return the defaul value, if one exists.
        Returns None if there is no value.
        """
        if self.binding is None:
            return None

        if isinstance(self.root_proxy, DeviceProxy):
            return self.binding.value
        else:
            attrs = self.binding.attributes
            return attrs.get(const.KARABO_SCHEMA_DEFAULT_VALUE)

    def get_history(self, time_start, time_end, max_value_count=-1):
        """Request the historical values of `binding` between `time_start` and
        `time_end` (XXX: inclusive?).

        NOTE: The reply comes via `self.binding`s `historic_data` Event trait.
        """
        if (self.binding is None or
                not isinstance(self.root_proxy, DeviceProxy)):
            return

        get_network().onGetPropertyHistory(self.root_proxy.device_id,
                                           self.path, time_start, time_end,
                                           max_value_count)

    def revert_edit(self):
        """Revert any local edits made to a property.
        """
        self.edit_value = None
        if self.binding is not None:
            self.binding.config_update = True

    def start_monitoring(self):
        """Tell the GUI server to start monitoring the device for our property
        """
        self.visible = True

        if not isinstance(self.root_proxy, DeviceProxy):
            return

        self.root_proxy.add_monitor()
        if self.pipeline_parent_path:
            self.root_proxy.connect_pipeline(self.pipeline_parent_path)

    def stop_monitoring(self):
        """Tell the GUI server to stop monitoring the device for our property
        """
        self.visible = False

        if not isinstance(self.root_proxy, DeviceProxy):
            return

        self.root_proxy.remove_monitor()
        if self.pipeline_parent_path:
            self.root_proxy.disconnect_pipeline(self.pipeline_parent_path)

    # -----------------------------------------------------------------------
    # Private and internal methods

    def __eq__(self, other):
        """Enable comparison of instances"""
        if not isinstance(other, PropertyProxy):
            return False
        return (self.root_proxy is other.root_proxy and
                self.path == other.path)

    def __hash__(self):
        """Traits objects need to be hashable, and defining __eq__ clears the
        default __hash__ implementation.
        """
        return id(self)

    def __repr__(self):
        return f"<{self.__class__.__name__} key={self.key}>"
