
# This is the global singleton for the Manager() function.
_manager = None


def getDevice(deviceId):
    """ Find the Device instance in the system topology with the ID `deviceId`.
    """
    from configuration import Configuration
    from .network import Network

    manager = Manager()

    device = manager.deviceData.get(deviceId)
    if device is None:
        device = manager.deviceData[deviceId] = Configuration(deviceId, 'device')
        device.updateStatus()
    if device.descriptor is None and device.status not in ("offline", "requested"):
        Network().onGetDeviceSchema(deviceId)
        device.status = "requested"
    return device


def getClass(serverId, classId):
    """ Find the class in the system topology from the server `serverId` with
    the ID `classId`.
    """
    from configuration import Configuration
    from .network import Network

    manager = Manager()

    cls = manager.serverClassData.get((serverId, classId))
    if cls is None:
        path = "{}.{}".format(serverId, classId)
        cls = manager.serverClassData[serverId, classId] = Configuration(path, 'class')

    if cls.descriptor is None or cls.status not in ("requested", "schema"):
        Network().onGetClassSchema(serverId, classId)
        cls.status = "requested"
    return cls


def Manager():
    """ Return the manager singleton.
    """
    from .manager import _Manager

    global _manager
    if _manager is None:
        _manager = _Manager()

    return _manager
