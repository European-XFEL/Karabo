from traits.api import HasStrictTraits, Dict, Event, List, Property, String

from karabo_gui.singletons.api import get_topology


def get_box(device_id, path):
    """ Return the Box for a given device and property path. """
    device_proxy = get_topology().get_device(device_id)
    path_parts = path.split(".")
    try:
        return device_proxy.getBox(path_parts)
    except AttributeError:
        # When a prpoerty is renamed, we get an AttributeError here.
        # Return None and deal with the consequences in the layers above.
        return None


class PendingBoxes(HasStrictTraits):
    """A collection of pending boxes for a widget which may or may not be
    attached to device properties that will appear in a future schema update.
    """
    # The keys for the boxes
    keys = List(String)
    # Fires once all of the boxes are ready to be used
    ready = Event
    # The boxes
    boxes = Property
    _complete = List
    _pending = Dict

    def destroy(self):
        """Clean up any signal/slot connections
        """
        self._complete = []

        for device in self._pending.keys():
            self._unsubscribe(device)
        self._pending = {}

    def _get_boxes(self):
        return self._complete

    def _keys_changed(self):
        self.destroy()

        for key in self.keys:
            device_id, path = key.split('.', 1)
            box = get_box(device_id, path)
            if box is None:
                device = get_topology().get_device(device_id)
                paths = self._pending.setdefault(device, [])
                paths.append(path)
            else:
                self._complete.append(box)

        assert len(self._pending) != 0
        for device in self._pending.keys():
            self._subscribe(device)

    def _pending_box_slot(self, device):
        """The object `device` just got a new schema
        """
        paths = self._pending[device]
        assert len(paths) != 0

        # Iterate over a copy so items can be removed from `paths` immediately
        paths_copy = paths[:]
        for path in paths_copy:
            box = get_box(device.id, path)
            if box is not None:
                self._complete.append(box)
                paths.remove(path)

        # Is that all for this device?
        if len(paths) == 0:
            del self._pending[device]
            self._unsubscribe(device)

        # Is that all for this widget?
        if len(self._pending) == 0:
            self.ready = True  # Tell the world

    def _subscribe(self, device):
        device.signalNewDescriptor.connect(self._pending_box_slot)
        device.addVisible()  # Start monitoring! This is important!

    def _unsubscribe(self, device):
        device.signalNewDescriptor.disconnect(self._pending_box_slot)
        device.removeVisible()
