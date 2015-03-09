from karabo.hash import Hash
from karabo.signalslot import slot
from karabo.python_device import Device

class DeviceClientBase(Device):
    def __init__(self, configuration):
        super().__init__(configuration)
        self.systemTopology = Hash("device", Hash(), "server", Hash(),
                                   "macro", Hash())

    def run(self):
        super().run()
        self._ss.emit("call", {"*": ["slotPing"]}, self.deviceId, 0, False)

    @slot
    def slotInstanceNew(self, instanceId, info):
        self.updateSystemTopology(instanceId, info, "instanceNew")
        super().slotInstanceNew(instanceId, info)

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        self.updateSystemTopology(instanceId, info, "instanceUpdated")
        super().slotInstanceUpdated(instanceId, info)

    @slot
    def slotInstanceGone(self, instanceId, info):
        self.systemTopology[info[type]].pop(instanceId, None)
        return super().slotInstanceGone(instanceId, info)

    @slot
    def slotPingAnswer(self, deviceId, info):
        self.updateSystemTopology(deviceId, info, None)

    def updateSystemTopology(self, instanceId, info, task):
        type = info["type"]
        ret = Hash(type, Hash())
        ret[type][instanceId] = Hash()
        ret[type][instanceId, ...] = dict(info.items())
        self.systemTopology.merge(ret)
        return ret


