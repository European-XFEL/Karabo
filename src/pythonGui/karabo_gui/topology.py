
from .singletons.api import get_manager, get_network


def getDevice(deviceId):
    """ Find the Device instance in the system topology with the ID `deviceId`.
    """
    from .configuration import Configuration

    manager = get_manager()
    network = get_network()

    device = manager.deviceData.get(deviceId)
    if device is None:
        device = Configuration(deviceId, 'device')
        manager.deviceData[deviceId] = device
        device.updateStatus()
    if device.descriptor is None and device.status not in ("offline", "requested"):
        network.onGetDeviceSchema(deviceId)
        device.status = "requested"
    return device


def getDeviceBox(box):
    """return a box that belongs to an active device

    if the box already is part of a running device, return it,
    if it is from a class in a project, return the corresponding
    instantiated device's box. """
    if box.configuration.type == "projectClass":
        return getDevice(box.configuration.id).getBox(box.path)
    return box


def getClass(serverId, classId):
    """ Find the class in the system topology from the server `serverId` with
    the ID `classId`.
    """
    from .configuration import Configuration

    manager = get_manager()
    network = get_network()

    klass = manager.serverClassData.get((serverId, classId))
    if klass is None:
        path = "{}.{}".format(serverId, classId)
        klass = Configuration(path, 'class')
        manager.serverClassData[serverId, classId] = klass

    if klass.descriptor is None or klass.status not in ("requested", "schema"):
        network.onGetClassSchema(serverId, classId)
        klass.status = "requested"
    return klass
