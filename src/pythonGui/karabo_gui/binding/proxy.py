from traits.api import (
    HasStrictTraits, Bool, DelegatesTo, Enum, Event, Instance, Int, Property,
    String, WeakRef, on_trait_change)

from karabo.common.api import DeviceStatus
from karabo_gui.singletons.api import get_network, get_topology
from .types import BaseBinding, BindingRoot, PipelineOutputBinding, SlotBinding

_ONLINE_STATUSES = (
    DeviceStatus.OK, DeviceStatus.ONLINE, DeviceStatus.ALIVE,
    DeviceStatus.MONITORING, DeviceStatus.SCHEMA, DeviceStatus.ERROR
)


class BaseDeviceProxy(HasStrictTraits):
    """Base class for proxying device classes, device instances, or project
    devices.
    """
    # The root of the data binding
    binding = Instance(BindingRoot)
    # The root's `state` property, if existent
    state_binding = Instance(BaseBinding, allow_none=True)
    # ID of the server hosting this class
    server_id = String
    # True when the device is online
    online = Property(Bool, depends_on='status')
    # The current status
    status = Enum(*DeviceStatus)
    # An event which fires when the schema is updated or otherwise changes
    schema_update = DelegatesTo('binding')

    @on_trait_change('binding.schema_update')
    def _binding_changed(self):
        """When `binding` is assigned to or its schema changes, refresh
        `state_binding`
        """
        self.state_binding = self.get_property_binding('state')

    def _get_online(self):
        return self.status in _ONLINE_STATUSES

    def _status_default(self):
        return DeviceStatus.OFFLINE

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


class DeviceProxy(BaseDeviceProxy):
    """The proxy for device instances and project devices. Instances may come
    on and offline.
    """
    # ID of the device we are proxying
    device_id = String
    # An event which fires when the configuration is updated from the network
    config_update = Event
    # The system tree node for this device
    topology_node = WeakRef('karabo_gui.topology.tree.SystemTreeNode',
                            allow_none=True)

    # A reference counter for tracking the monitoring
    _monitor_count = Int(0)

    # -----------------------------------------------------------------------
    # Traits methods

    def _config_update_fired(self):
        if self.status is DeviceStatus.SCHEMA:
            self.status = DeviceStatus.ALIVE
        if self.status is DeviceStatus.ALIVE and self._monitor_count > 0:
            self.status = DeviceStatus.MONITORING

    def _schema_update_fired(self):
        if self.status is DeviceStatus.REQUESTED:
            if self._monitor_count > 0:
                self._start_monitoring()
            self.status = DeviceStatus.SCHEMA
        elif self.status in (DeviceStatus.ALIVE, DeviceStatus.MONITORING):
            get_network().onGetDeviceConfiguration(self.device_id)

    def _status_changed(self, new):
        if new is DeviceStatus.ONLINE and self._monitor_count > 0:
            self.refresh_schema()

    # -----------------------------------------------------------------------
    # Public interface

    def add_monitor(self):
        """Ask the GUI server to begin monitoring this device
        """
        ignored_statuses = (DeviceStatus.OFFLINE, DeviceStatus.REQUESTED)

        self._monitor_count += 1
        if self._monitor_count == 1:
            if self.status is DeviceStatus.ONLINE:
                self.refresh_schema()
            elif self.status not in ignored_statuses:
                self._start_monitoring()

    def remove_monitor(self):
        """Ask the GUI server to stop monitoring this device
        """
        assert self._monitor_count > 0
        self._monitor_count -= 1
        if self._monitor_count == 0:
            get_network().onStopMonitoringDevice(self.device_id)
            if self.status is DeviceStatus.MONITORING:
                self.status = DeviceStatus.ALIVE

    def connect_pipeline(self, path):
        """Ask the GUI server to subscribe to a pipeline output
        """
        # XXX: onSubscribeToOutput() has a different signature than this!
        get_network().onSubscribeToOutput(self.device_id, path, True)

    def disconnect_pipeline(self, path):
        """Ask the GUI server to unsubscribe from a pipeline output
        """
        # XXX: onSubscribeToOutput() has a different signature than this!
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
        if self.status is not DeviceStatus.REQUESTED:
            get_network().onGetDeviceSchema(self.device_id)
            self.status = DeviceStatus.REQUESTED

    # -----------------------------------------------------------------------
    # Private interface

    def _start_monitoring(self):
        get_network().onStartMonitoringDevice(self.device_id)


class DeviceClassProxy(BaseDeviceProxy):
    """The proxy for device classes. Used for editing before instantiation.
    """

    def _status_default(self):
        topology = get_topology()
        server_key = 'server.{}'.format(self.server_id)
        attributes = topology.get_attributes(server_key)
        if attributes is None:
            return DeviceStatus.NOSERVER
        elif self.binding.class_id not in attributes.get('deviceClasses', []):
            return DeviceStatus.NOPLUGIN
        return DeviceStatus.OFFLINE

    def _schema_update_fired(self):
        if self.status is DeviceStatus.REQUESTED:
            self.status = self._status_default()

    def refresh_schema(self):
        """Request a recent schema for this device class
        """
        if self.status is not DeviceStatus.REQUESTED:
            get_network().onGetClassSchema(self.server_id,
                                           self.binding.class_id)
            self.status = DeviceStatus.REQUESTED


class PropertyProxy(HasStrictTraits):
    """A proxy for a single device property
    """
    # Full path of the property
    path = String
    # The binding for the property
    binding = Instance(BaseBinding, allow_none=True)
    # The value for the property (from the binding instance)
    value = DelegatesTo('binding')
    # Parent device or class proxy
    root_proxy = Instance(BaseDeviceProxy)
    # Potential parent path if `binding` is a child of a Pipeline Output
    pipeline_parent_path = String

    # -----------------------------------------------------------------------
    # Traits methods

    def _pipeline_parent_path_default(self):
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

    @on_trait_change('root_proxy.schema_update,path')
    def _binding_update(self):
        if self.root_proxy is None:
            return
        self.binding = self.root_proxy.get_property_binding(self.path)
        self.pipeline_parent_path = self._pipeline_parent_path_default()

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
            get_network().onExecute(self.root_proxy.device_id, self.path)

    def get_history(self, time_start, time_end, max_value_count=-1):
        """Request the historical values of `binding` between `time_start` and
        `time_end` (XXX: inclusive?).

        NOTE: The reply comes via `self.binding`s `historic_data` Event trait.
        """
        if (self.binding is None or
                not isinstance(self.root_proxy, DeviceProxy)):
            return

        address = self.root_proxy.device_id + ':' + self.path
        get_network().onGetPropertyHistory(address,
                                           time_start, time_end,
                                           max_value_count)

    def start_monitoring(self):
        """Tell the GUI server to start monitoring the device for our property
        """
        if not isinstance(self.root_proxy, DeviceProxy):
            return

        self.root_proxy.add_monitor()
        if self.pipeline_parent_path:
            self.root_proxy.connect_pipeline(self.pipeline_parent_path)

    def stop_monitoring(self):
        """Tell the GUI server to stop monitoring the device for our property
        """
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
