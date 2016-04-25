

def getDevice(deviceId):
    """ Find the Device instance in the system topology with the ID `deviceId`.
    """
    from configuration import Configuration
    from manager import Manager
    from .network import Network

    manager = Manager()
    network = Network()

    c = manager.deviceData.get(deviceId)
    if c is None:
        c = manager.deviceData[deviceId] = Configuration(deviceId, 'device')
        c.updateStatus()
    if c.descriptor is None and c.status not in ("offline", "requested"):
        network.onGetDeviceSchema(deviceId)
        c.status = "requested"
    return c


def getClass(serverId, classId):
    """ Find the class in the system topology from the server `serverId` with
    the ID `classId`.
    """
    from configuration import Configuration
    from manager import Manager
    from .network import Network

    manager = Manager()
    network = Network()

    c = manager.serverClassData.get((serverId, classId))
    if c is None:
        path = "{}.{}".format(serverId, classId)
        c = manager.serverClassData[serverId, classId] = Configuration(path, 'class')

    if c.descriptor is None or c.status not in ("requested", "schema"):
        network.onGetClassSchema(serverId, classId)
        c.status = "requested"
    return c
