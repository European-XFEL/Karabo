from PyQt4.QtCore import QPoint
from traits.api import (
    HasStrictTraits, Bool, Dict, Enum, Event, Instance, Int, List, Property,
    String, cached_property, on_trait_change
)

from karabo.common.scenemodel.api import WorkflowItemModel
from karabogui.binding.api import NodeBinding, PropertyProxy
from karabogui.singletons.api import get_topology
from karabogui.topology.api import ProjectDeviceInstance
from . import const
from .utils import get_curve_points


class WorkflowChannelModel(HasStrictTraits):
    """A model for workflow channels (inputs or outputs).
    """
    # The proxy for this channel
    proxy = Instance(PropertyProxy)
    # The device IDs for this channel
    device_ids = Property(List(String))
    # The index of this channel within the WorkflowItem
    index = Int(0)
    # The type of this channel
    kind = Enum(const.CHANNEL_INPUT, const.CHANNEL_OUTPUT)
    # The item associated with this channel
    model = Instance(WorkflowItemModel)
    # The scene position of the channel
    position = Property(Instance(QPoint),
                        depends_on=['model.x', 'model.y', 'model.height',
                                    'model.width'])

    def _get_device_ids(self):
        # XXX: There's only one ID until workflow group items are supported
        return [self.model.device_id]

    @cached_property
    def _get_position(self):
        model = self.model
        y = model.y + const.CHANNEL_HEIGHT * self.index
        if self.kind == const.CHANNEL_INPUT:
            x = model.x - const.CONNECTION_OFFSET
        else:
            x = model.x + model.width + const.CONNECTION_OFFSET
        return QPoint(x, y)


class WorkflowConnectionModel(HasStrictTraits):
    """A model for connections between channels
    """
    # The distribution type of the connection
    data_distribution = Enum('', const.DATA_DIST_SHARED, const.DATA_DIST_COPY)
    # The input and output sides of the connection
    input = Instance(WorkflowChannelModel)
    output = Instance(WorkflowChannelModel)
    # Convenience traits for the input and output positions
    input_pos = Property(Instance(QPoint))
    output_pos = Property(Instance(QPoint))
    # Screen points for the curve representing this connection
    curve_points = Property(List(Instance(QPoint)),
                            depends_on=['input.position', 'output.position'])

    @cached_property
    def _get_curve_points(self):
        """These points are intended to be the control points for a
        cubic bezier curve.
        """
        return get_curve_points(self.output_pos, self.input_pos)

    def _get_input_pos(self):
        return self.input.position

    def _get_output_pos(self):
        return self.output.position


class WorkflowDeviceModel(HasStrictTraits):
    """A model for workflow item device status.
    """
    # Online/Offline device
    device = Instance(ProjectDeviceInstance)
    # The item model associated with this device status
    model = Instance(WorkflowItemModel)
    # The inputs and outputs of the device
    inputs = List(Instance(WorkflowChannelModel))
    outputs = List(Instance(WorkflowChannelModel))
    # Let SceneWorkflowModel know to refresh its connections
    check_connection = Event
    # The scene position of the device status icon
    position = Property(Instance(QPoint),
                        depends_on=['model.x', 'model.y', 'model.height',
                                    'model.width'])
    # Are we visible?
    _visible = Bool(False)

    # --------------------------------------------
    # Public interface

    def set_visible(self, visible):
        """Subscribe/Unsubscribe from device updates on boxes
        """
        if self._visible == visible:
            return  # no change. nothing to do.

        self._visible = visible

        if self.device is not None:
            if visible:
                self.device.start_monitoring()
            else:
                self.device.stop_monitoring()

    # --------------------------------------------
    # Traits handlers

    @cached_property
    def _get_position(self):
        model = self.model
        return QPoint(model.x, model.y)

    def _device_changed(self, old, new):
        """When a device is added/removed, make sure it is monitored.
        """
        if old is not None and self._visible:
            old.stop_monitoring()
        if new is not None and self._visible:
            new.start_monitoring()

    def _model_changed(self):
        """When the model changes, set the project device instance.
        """
        if self.model is None:
            self.device = None
            return

        self.device = get_topology().get_project_device(self.model.device_id)

    @on_trait_change('device.online,device.schema_update')
    def _refresh_channels(self):
        """When the project device changes, or it changes between
        online/offline states: refresh the workflow channels.
        """
        if self.device is None or len(self.device.proxy.binding.value) == 0:
            # No device, or the device has no schema assigned
            self.inputs = []
            self.outputs = []
            self.check_connection = True
            return

        # Collect all known channels for the current Device[Class]Proxy
        device = self.device.proxy
        inputs, outputs = self._collect_channels(device, device.binding)

        # If a channel was already found before, don't replace it with a new
        # instance. Otherwise the swapping of "identical" WorkflowChannelModel
        # objects will cause an infinite add_monitor/remove_monitor feedback
        # loop with the GUI server.
        def _avoid_duplicates(found, existing):
            found = {ch.proxy.key: ch for ch in found}
            existing = {ch.proxy.key: ch for ch in existing}
            channels = [existing[key] if key in existing else value
                        for key, value in found.items()]
            channels.sort(key=lambda x: x.proxy.key)
            return channels

        self.inputs = _avoid_duplicates(inputs, self.inputs)
        self.outputs = _avoid_duplicates(outputs, self.outputs)
        self.check_connection = True

    def _collect_channels(self, device, binding, *,
                          path='', inputs=None, outputs=None):
        """Recursively find all input/output channels for a device
        """
        inputs = inputs or []
        outputs = outputs or []
        for name in binding.value:
            # The child binding for `name`:
            sub_binding = getattr(binding.value, name, None)
            if sub_binding is None:
                continue

            # Now we check whether it has a display type. If yes, check for
            # input/output channel and append in case.
            # If not, we might have a node (i.e. NodeBinding).
            display_type = sub_binding.display_type
            sub_path = (path + '.' if path else '') + name
            if display_type == const.CHANNEL_INPUT:
                channel = WorkflowChannelModel(
                    proxy=PropertyProxy(root_proxy=device, path=sub_path),
                    model=self.model,
                    kind=const.CHANNEL_INPUT, index=len(inputs))
                inputs.append(channel)
            elif display_type == const.CHANNEL_OUTPUT:
                channel = WorkflowChannelModel(
                    proxy=PropertyProxy(root_proxy=device, path=sub_path),
                    model=self.model,
                    kind=const.CHANNEL_OUTPUT, index=len(outputs))
                outputs.append(channel)
            elif isinstance(sub_binding, NodeBinding):
                # A node that we have to recurse into:
                self._collect_channels(device, sub_binding, path=sub_path,
                                       inputs=inputs, outputs=outputs)
        return inputs, outputs


class SceneWorkflowModel(HasStrictTraits):
    """A model for tracking workflow connections in the SceneView.
    """
    # All channels in the model
    channels = Property(List(Instance(WorkflowChannelModel)),
                        depends_on=['input_channels', 'output_channels'])
    # The connections between the channels
    connections = List(Instance(WorkflowConnectionModel))
    # An event triggered when the connections or channels change
    updated = Event

    # Ths dictionary of devices backing the workflow items
    devices = List(Instance(WorkflowDeviceModel))
    # The list of current input channels
    input_channels = List(Instance(WorkflowChannelModel))
    # The list of current output channels
    output_channels = List(Instance(WorkflowChannelModel))

    # All of the WorkflowItemModel objects in the scene.
    _workflow_items = List(Instance(WorkflowItemModel))
    _device_map = Dict  # WorkflowItemModel -> WorkflowDeviceModel

    # Are we visible?
    _scene_visible = Bool(False)

    # --------------------------------------------
    # Public interface

    def add_items(self, items):
        """Add WorkflowItemModels to be monitored."""
        self._workflow_items.extend(items)

    def remove_items(self, items):
        """Remove WorkflowItemModels from monitoring"""
        for it in items:
            self._workflow_items.remove(it)

    def destroy(self):
        """Clean up any channels and devices"""
        # Cause a single items_changed notification. The handler will clean up
        # the channels and devices.
        self._workflow_items[:] = []

    def set_visible(self, visible):
        """Subscribe/Unsubscribe from device updates
        """
        # Keep the state for later adds/removes
        self._scene_visible = visible

        for device in self.devices:
            device.set_visible(visible)

    # --------------------------------------------
    # Traits handlers

    @cached_property
    def _get_channels(self):
        return self.input_channels + self.output_channels

    @on_trait_change('connections,input_channels,output_channels,devices,'
                     'connections:data_distribution')
    def _needs_update(self):
        # Event traits don't have a value, they just generate notifications
        self.updated = True

    @on_trait_change('devices:inputs,devices:outputs,devices:check_connection')
    def _refresh_channels(self, obj, name, old, new):
        if name == 'inputs':
            self._remove_device_channels(old, [])
            self._add_device_channels(new, [])
        elif name == 'outputs':
            self._remove_device_channels([], old)
            self._add_device_channels([], new)
        elif name == 'check_connection':
            self._refresh_connections()

    def _devices_items_changed(self, event):
        """A trait notification handler for the `devices_items` list.
        """
        for device in event.added:
            device.set_visible(self._scene_visible)
        for device in event.removed:
            # It's safe to just tell every removed device that it's not visible
            device.set_visible(False)

    def __workflow_items_items_changed(self, event):
        """A trait notification handler for the `_workflow_items` list.
        """
        for model in event.added:
            device = WorkflowDeviceModel(model=model)
            self.devices.append(device)
            self._device_map[model] = device
            self._add_device_channels(device.inputs, device.outputs)

        for model in event.removed:
            device = self._device_map.pop(model, None)
            if device is not None:
                self.devices.remove(device)
                self._remove_device_channels(device.inputs, device.outputs)

        self._refresh_connections()

    # --------------------------------------------
    # Private interface

    def _add_device_channels(self, inputs, outputs):
        for ch in inputs:
            self._subscribe_to_input_changes(ch)
        self.input_channels.extend(inputs)
        self.output_channels.extend(outputs)

    def _remove_device_channels(self, inputs, outputs):
        for ch in inputs:
            if ch not in self.input_channels:
                continue
            self._unsubscribe_from_input_changes(ch)
            self.input_channels.remove(ch)
        for ch in outputs:
            if ch not in self.output_channels:
                continue
            self.output_channels.remove(ch)

    def _connected_outputs_cb(self):
        """Callback with the connected outputs of an input channel changes.
        """
        # XXX: Be less ham-fisted about this later.
        self._refresh_connections()

    def _data_distribution_cb(self, obj, name, value):
        """Callback when the data distribution type of a channel changes
        """
        inputs = [ch for ch in self.input_channels
                  if ch.proxy.binding.value.dataDistribution is obj]
        for conn in self.connections:
            if conn.input in inputs:
                conn.data_distribution = obj.value

    def _lookup_output(self, device_id, path):
        for output in self.output_channels:
            if output.model.device_id == device_id:
                if output.proxy.path == path:
                    return output
        return None

    def _refresh_connections(self):
        connections = []
        for input in self.input_channels:
            binding = input.proxy.binding
            if (binding is None or
                    'connectedOutputChannels' not in binding.value):
                continue

            connected_outputs = binding.value.connectedOutputChannels.value
            if not connected_outputs:
                continue

            data_dist = ''
            if 'dataDistribution' in binding.value:
                data_dist = binding.value.dataDistribution.value

            for output in connected_outputs:
                output_dev_id, output_path = output.split(':')
                output = self._lookup_output(output_dev_id, output_path)
                if output is not None:
                    conn = WorkflowConnectionModel(input=input, output=output,
                                                   data_distribution=data_dist)
                    connections.append(conn)
        self.connections = connections

    def _subscribe_to_input_changes(self, input_channel):
        """Subscribe to updates from a channel node's properties."""
        binding = input_channel.proxy.binding
        connected_outputs = binding.value.connectedOutputChannels
        connected_outputs.on_trait_change(self._connected_outputs_cb,
                                          'config_update')
        if 'dataDistribution' in binding.value:
            data_dist = binding.value.dataDistribution
            data_dist.on_trait_change(self._data_distribution_cb,
                                      'config_update')

    def _unsubscribe_from_input_changes(self, input_channel):
        """Unsubscribe from property updates."""
        binding = input_channel.proxy.binding
        connected_outputs = binding.value.connectedOutputChannels
        connected_outputs.on_trait_change(self._connected_outputs_cb,
                                          'config_update', remove=True)
        if 'dataDistribution' in binding.value:
            data_dist = binding.value.dataDistribution
            data_dist.on_trait_change(self._data_distribution_cb,
                                      'config_update', remove=True)
