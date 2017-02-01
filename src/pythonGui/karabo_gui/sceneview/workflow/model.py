from PyQt4.QtCore import QPoint
from traits.api import (
    HasStrictTraits, Bool, Dict, Enum, Event, Instance, Int, List, Property,
    String, cached_property, on_trait_change
)

from karabo.common.scenemodel.api import WorkflowItemModel
from karabo_gui.schema import Box, Dummy
from karabo_gui.singletons.api import get_topology
from karabo_gui.topology.api import ProjectDeviceInstance
from .const import (
    CHANNEL_INPUT, CHANNEL_OUTPUT, CHANNEL_HEIGHT, CONNECTION_OFFSET,
    DATA_DIST_COPY, DATA_DIST_SHARED)
from .utils import get_curve_points


class WorkflowChannelModel(HasStrictTraits):
    """ A model for workflow channels (inputs or outputs).
    """
    # The box for this channel
    box = Instance(Box)
    # The device IDs for this channel
    device_ids = Property(List(String))
    # The index of this channel within the WorkflowItem
    index = Int(0)
    # The type of this channel
    kind = Enum(CHANNEL_INPUT, CHANNEL_OUTPUT)
    # The item associated with this channel
    model = Instance(WorkflowItemModel)
    # The scene position of the channel
    position = Property(Instance(QPoint),
                        depends_on=['model.x', 'model.y', 'model.height',
                                    'model.width'])

    def toggle_box_notifications(self, visible):
        """Register/Unregister for box notifications.
        """
        if visible:
            self.box.addVisible()
        else:
            self.box.removeVisible()

    def _get_device_ids(self):
        # XXX: There's only one ID until workflow group items are supported
        return [self.model.device_id]

    @cached_property
    def _get_position(self):
        model = self.model
        y = model.y + CHANNEL_HEIGHT * self.index
        if self.kind == CHANNEL_INPUT:
            x = model.x - CONNECTION_OFFSET
        else:
            x = model.x + model.width + CONNECTION_OFFSET
        return QPoint(x, y)


class WorkflowConnectionModel(HasStrictTraits):
    """ A model for connections between channels
    """
    # The distribution type of the connection
    data_distribution = Enum('', DATA_DIST_SHARED, DATA_DIST_COPY)
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
        """ These points are intended to be the control points for a
        cubic bezier curve.
        """
        return get_curve_points(self.output_pos, self.input_pos)

    def _get_input_pos(self):
        return self.input.position

    def _get_output_pos(self):
        return self.output.position


class WorkflowDeviceModel(HasStrictTraits):
    """ A model for workflow item device status.
    """
    # Online/Offline device
    device = Instance(ProjectDeviceInstance)
    # The item model associated with this device status
    model = Instance(WorkflowItemModel)
    # The inputs and outputs of the device
    inputs = List(Instance(WorkflowChannelModel))
    outputs = List(Instance(WorkflowChannelModel))
    # The scene position of the device status icon
    position = Property(Instance(QPoint),
                        depends_on=['model.x', 'model.y', 'model.height',
                                    'model.width'])

    @cached_property
    def _get_position(self):
        model = self.model
        x = model.x
        y = model.y + model.height / 3
        return QPoint(x, y)

    def _model_changed(self):
        """When the model changes, set the project device instance.
        """
        if self.model is None:
            self.device = None
            return

        self.device = get_topology().get_project_device(self.model.device_id)

    @on_trait_change('device.online,device.boxes_updated')
    def _refresh_channels(self):
        """When the project device changes, or it changes between
        online/offline states: refresh the workflow channels.
        """
        if self.device is None:
            self.inputs = self.outputs = []
            return

        box = self.device.current_configuration
        self.inputs, self.outputs = self._find_channels(box)

    def _find_channels(self, box, inputs=None, outputs=None):
        """ Recursively find all input/output channels from a box
        """
        inputs = inputs or []
        outputs = outputs or []
        if box.descriptor is not None:
            for key, value in box.descriptor.dict.items():
                # The child box belonging to key:
                sub_box = getattr(box.boxvalue, key, None)
                if sub_box is None:
                    continue
                # Now we check whether it is a display type. If yes, check for
                # input/output channel and append in case.
                # If not, we might have a node (i.e. Schema).
                displayType = sub_box.descriptor.displayType
                if displayType is None:
                    if hasattr(value, 'dict'):
                        # A Schema node that we have to dive into:
                        self._find_channels(sub_box, inputs=inputs,
                                            outputs=outputs)
                elif displayType == CHANNEL_INPUT:
                    channel = WorkflowChannelModel(
                        box=sub_box, model=self.model,
                        kind=CHANNEL_INPUT, index=len(inputs))
                    inputs.append(channel)
                elif displayType == CHANNEL_OUTPUT:
                    channel = WorkflowChannelModel(
                        box=sub_box, model=self.model,
                        kind=CHANNEL_OUTPUT, index=len(outputs))
                    outputs.append(channel)
        return inputs, outputs


class SceneWorkflowModel(HasStrictTraits):
    """ A model for tracking workflow connections in the SceneView.
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

    def get_item(self, device_id):
        """ This method checks whether a workflow item with the given
            ``device_id`` already exists.
            If that it the case the item is returned, else None.
        """
        for item in self._workflow_items:
            if item.device_id == device_id:
                return item
        return None

    def add_items(self, items):
        """ Add WorkflowItemModels to be monitored. """
        self._workflow_items.extend(items)

    def remove_items(self, items):
        """ Remove WorkflowItemModels from monitoring """
        for it in items:
            self._workflow_items.remove(it)

    def destroy(self):
        """ Clean up any channels (causing their Boxes to disconnect). """
        # Cause a single items_changed notification. The handler will clean up
        # the channels.
        self._workflow_items[:] = []

    def set_visible(self, visible):
        """Subscribe/Unsubscribe from device updates on boxes
        """
        for ch in self.channels:
            ch.toggle_box_notifications(visible)

        # Keep the state for later adds/removes
        self._scene_visible = visible

    # --------------------------------------------
    # Traits handlers

    @cached_property
    def _get_channels(self):
        return self.input_channels + self.output_channels

    @on_trait_change('connections,input_channels,output_channels,devices')
    def _needs_update(self):
        # Event traits don't have a value, they just generate notifications
        self.updated = True

    @on_trait_change('devices:inputs,devices:outputs')
    def _refresh_channels(self, obj, name, old, new):
        if name == 'inputs':
            self._remove_device_channels(old, [])
            self._add_device_channels(new, [])
        elif name == 'outputs':
            self._remove_device_channels([], old)
            self._add_device_channels([], new)

    def __workflow_items_items_changed(self, event):
        """ A trait notification handler for the `_workflow_items` list.
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

    # --------------------------------------------
    # Private interface

    def _add_device_channels(self, inputs, outputs):
        for ch in inputs:
            if self._scene_visible:
                ch.toggle_box_notifications(True)
            self._subscribe_to_input_changes(ch)
        for ch in outputs:
            if self._scene_visible:
                ch.toggle_box_notifications(True)
        self.input_channels.extend(inputs)
        self.output_channels.extend(outputs)

    def _remove_device_channels(self, inputs, outputs):
        for ch in inputs:
            if self._scene_visible:
                ch.toggle_box_notifications(False)
            self._unsubscribe_from_input_changes(ch)
            self.input_channels.remove(ch)
        for ch in outputs:
            if self._scene_visible:
                ch.toggle_box_notifications(False)
            self.output_channels.remove(ch)

    def _connected_outputs_cb(self, box, value, timestamp):
        """ Callback with the connected outputs of an input channel changes.
        """
        # XXX: Be less ham-fisted about this later.
        self._refresh_connections()

    def _data_distribution_cb(self, box, value, timestamp):
        """ Callback when the data distribution type of a channel changes
        """
        inputs = [ch for ch in self.input_channels if ch.box is box]
        for conn in self.connections:
            if conn.input in inputs:
                conn.data_distribution = value

    def _lookup_output(self, device_id, box_path):
        for output in self.output_channels:
            if output.model.device_id == device_id:
                if '.'.join(output.box.path) == box_path:
                    return output
        return None

    def _refresh_connections(self):
        connections = []
        for input in self.input_channels:
            path = input.box.path + ('connectedOutputChannels',)
            configuration = input.box.configuration
            if not configuration.hasBox(path):
                continue

            connected_outputs = configuration.getBox(path).value
            if isinstance(connected_outputs, Dummy):
                continue

            data_dist = ''
            if hasattr(input.box.value, "dataDistribution"):
                data_dist = input.box.boxvalue.dataDistribution.value

            for output in connected_outputs:
                output_dev_id, output_path = output.split(':')
                output = self._lookup_output(output_dev_id, output_path)
                if output is not None:
                    conn = WorkflowConnectionModel(input=input, output=output,
                                                   data_distribution=data_dist)
                    connections.append(conn)
        self.connections = connections

    def _subscribe_to_input_changes(self, input):
        """ Subscribe to updates from a channel's box. """
        connected_outputs = input.box.boxvalue.connectedOutputChannels
        connected_outputs.signalUpdateComponent.connect(
            self._connected_outputs_cb)
        if hasattr(input.box.value, "dataDistribution"):
            data_dist = input.box.boxvalue.dataDistribution
            data_dist.signalUpdateComponent.connect(self._data_distribution_cb)

    def _unsubscribe_from_input_changes(self, input):
        """ Unsubscribe from box updates. """
        connected_outputs = input.box.boxvalue.connectedOutputChannels
        connected_outputs.signalUpdateComponent.disconnect(
            self._connected_outputs_cb)
        if hasattr(input.box.value, "dataDistribution"):
            data_dist = input.box.boxvalue.dataDistribution
            data_dist.signalUpdateComponent.disconnect(
                self._data_distribution_cb)
