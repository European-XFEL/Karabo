from itertools import chain

from PyQt4.QtCore import QPoint
from traits.api import (HasStrictTraits, Any, Bool, Dict, Enum, Event,
                        Instance, Int, List, Property, String,
                        cached_property, on_trait_change)

from karabo_gui.scenemodel.api import WorkflowItemModel
from karabo_gui.schema import Dummy
from karabo_gui.topology import getDevice
from .const import (
    CHANNEL_INPUT, CHANNEL_OUTPUT, CHANNEL_HEIGHT, CONNECTION_OFFSET,
    DATA_DIST_COPY, DATA_DIST_SHARED)
from .utils import get_curve_points


class WorkflowDeviceStatusModel(HasStrictTraits):
    """ A model for workflow item device status.
    """
    # The box for the device
    box = Any
    # The status of the device
    status = String('offline')
    # If device is in error
    error = Bool
    # The item model associated with this device status
    model = Instance(WorkflowItemModel)
    # The scene position of the device status
    position = Property(Instance(QPoint),
                        depends_on=['model.x', 'model.y', 'model.height',
                                    'model.width'])

    @cached_property
    def _get_position(self):
        model = self.model
        x = model.x
        y = model.y + model.height / 3
        return QPoint(x, y)


class WorkflowChannelModel(HasStrictTraits):
    """ A model for workflow channels (inputs or outputs).
    """
    # The box for this channel
    box = Any
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

    def toggle_box_notifications(self, state):
        """ Register/Unregister for box notifications. """
        if state:
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

    # Ths list of current device states
    device_states = List(Instance(WorkflowDeviceStatusModel))
    # The list of current input channels
    input_channels = List(Instance(WorkflowChannelModel))
    # The list of current output channels
    output_channels = List(Instance(WorkflowChannelModel))

    # A private mapping of item model -> _DeviceEntry objects
    _devices = Dict
    # All of the WorkflowItemModel objects in the scene.
    _workflow_items = List(Instance(WorkflowItemModel))

    # --------------------------------------------
    # Public interface

    def add_items(self, items):
        """ Add WorkflowItemModels to be monitored. """
        self._workflow_items.extend(items)
        # Collect the remaining device states
        self._refresh_device_states()
        # Collect the remaining inputs and outputs
        self._refresh_channels()

    def remove_items(self, items):
        """ Remove WorkflowItemModels from monitoring """
        for it in items:
            self._workflow_items.remove(it)
        # Collect the remaining device states
        self._refresh_device_states()
        # Collect the remaining inputs and outputs
        self._refresh_channels()

    def destroy(self):
        """ Clean up any channels (causing their Boxes to disconnect). """
        # Cause a single items_changed notification. The handler will clean up
        # the channels.
        self._workflow_items[:] = []

    # --------------------------------------------
    # Traits handlers

    @cached_property
    def _get_channels(self):
        return self.input_channels + self.output_channels

    @on_trait_change('connections,input_channels,output_channels,'
                     'device_states')
    def _needs_update(self):
        # Event traits don't have a value, they just generate notifications
        self.updated = True

    def __workflow_items_items_changed(self, event):
        """ A trait notification handler for the `_workflow_items` list """
        def toggle_boxes(entry, state):
            device_status = entry.device_status
            if state:
                self._subscribe_device_status(device_status)
            else:
                self._unsubscribe_device_status(device_status)
            for ch in chain(entry.inputs, entry.outputs):
                ch.toggle_box_notifications(state)
            for ch in entry.inputs:
                if state:
                    self._subscribe_to_input_changes(ch)
                else:
                    self._unsubscribe_from_input_changes(ch)

        for item_model in event.added:
            entry = _get_device_entry(item_model)
            self._devices[item_model] = entry
            toggle_boxes(entry, True)

        for item_model in event.removed:
            if item_model in self._devices:
                entry = self._devices.pop(item_model)
                toggle_boxes(entry, False)

    # --------------------------------------------
    # Private interface

    def _lookup_output(self, device_id, box_path):
        for output in self.output_channels:
            if output.model.device_id == device_id:
                if '.'.join(output.box.path) == box_path:
                    return output
        return None

    def _refresh_channels(self):
        entries = list(self._devices.values())
        self.input_channels = [c for entry in entries for c in entry.inputs]
        self.output_channels = [c for entry in entries for c in entry.outputs]

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

    def _refresh_device_states(self):
        entries = list(self._devices.values())
        self.device_states = [entry.device_status for entry in entries]

    def _subscribe_device_status(self, device_status):
        """ Subscribe from device box updates. """
        device_box = device_status.box
        device_box.signalStatusChanged.connect(self._device_status_changed)

    def _unsubscribe_device_status(self, device_status):
        """ Unsubscribe from device box updates. """
        device_box = device_status.box
        device_box.signalStatusChanged.disconnect(self._device_status_changed)

    def _device_status_changed(self, device_box, status, error):
        """ Callback when the status of the device is changes.
        """
        for ds in self.device_states:
            if ds.box is device_box:
                ds.status = status
                ds.error = error


class _DeviceEntry(HasStrictTraits):
    """ A private convenience class for tracking devices
    """
    device_id = String
    box = Any
    device_status = Any
    inputs = List
    outputs = List


def _get_channels(model, box, inputs=None, outputs=None):
    """ Recursively find all input/output channels from a box """
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
                    _get_channels(model, sub_box,
                                  inputs=inputs, outputs=outputs)
            elif displayType == CHANNEL_INPUT:
                inputs.append(WorkflowChannelModel(box=sub_box, model=model,
                                                   kind=CHANNEL_INPUT,
                                                   index=len(inputs)))
            elif displayType == CHANNEL_OUTPUT:
                outputs.append(WorkflowChannelModel(box=sub_box, model=model,
                                                    kind=CHANNEL_OUTPUT,
                                                    index=len(outputs)))
    return inputs, outputs


def _get_device_entry(model):
    """ Build a _DeviceEntry for a given WorkflowItemModel. """
    device = getDevice(model.device_id)
    device_status = WorkflowDeviceStatusModel(box=device, model=model)
    inputs, outputs = _get_channels(model, device)
    return _DeviceEntry(device_id=model.device_id, box=device,
                        device_status=device_status,
                        inputs=inputs, outputs=outputs)
