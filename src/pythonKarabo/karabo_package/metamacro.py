from asyncio import coroutine, gather

from karabo.python_device import Device
from karabo.hashtypes import String
from karabo.macro import Macro


class MetaMacro(Device):
    code = String()

    def __init__(self, configuration):
        super().__init__(configuration)
        Macro.subclasses = []
        exec(self.code, globals())
        self.classes = Macro.subclasses
        Macro.subclasses = []

    @coroutine
    def run_async(self):
        p = dict(_serverId_=self.serverId)
        objs = []
        for c in self.classes:
            p["_deviceId_"] ="{}-{}".format(self.deviceId, c.__name__)
            objs.append(c(p))
        yield from gather(*[o.run_async() for o in objs])
