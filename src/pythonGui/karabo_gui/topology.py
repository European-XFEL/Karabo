
from .singletons.api import get_topology


def getDevice(deviceId):
    """ Find the Device instance in the system topology with the ID `deviceId`.
    """
    return get_topology().get_device(deviceId)


def getDeviceBox(box):
    """Return a box that belongs to an active device

    if the box already is part of a running device, return it,
    if it is from a class in a project, return the corresponding
    instantiated device's box.
    """
    if box.configuration.type == "projectClass":
        return get_topology().get_device(box.configuration.id).getBox(box.path)
    return box


def getClass(serverId, classId):
    """ Find the class in the system topology from the server `serverId` with
    the ID `classId`.
    """
    return get_topology().get_class(serverId, classId)
