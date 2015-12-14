from karabo.api2.hash import Hash


class DeviceData(object):
    """ This class represents a device
    """

    def __init__(self, serverId, classId, deviceId, ifexists, config=None):
        assert ifexists in ("ignore", "restart")

        self.name = deviceId
        self.serverId = serverId
        self.classId = classId
        self.ifexists = ifexists
        self.config = config or Hash()

    def serialize(self):
        wrapper = Hash(self.classId, self.config)
        return wrapper.encode('XML')


class DeviceGroupData(DeviceData):
    """ This class represents a list of devices and is a device itself.
    """

    def __init__(self, *args, **kwargs):
        super(DeviceGroupData, self).__init__(*args, **kwargs)
        self.devices = []

    def addDevice(self, device):
        self.devices.append(device)
