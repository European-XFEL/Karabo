from asyncio import coroutine, gather

from karabo.python_device import Device
from karabo.hashtypes import String
from karabo.macro import Macro


class MetaMacro(Device):
    code = String()
    project = String()
    module = String()

    def __init__(self, configuration):
        super().__init__(configuration)
        Macro.subclasses = []
        code = compile(self.code, self.module, "exec")
        exec(code, globals())
        self.classes = Macro.subclasses
        Macro.subclasses = []

    @coroutine
    def run_async(self):
        p = dict(_serverId_=self.serverId, project=self.project,
                 module=self.module)
        objs = []
        for c in self.classes:
            p["_deviceId_"] ="{}-{}".format(self.deviceId, c.__name__)
            objs.append(c(p))
        yield from gather(*[o.startInstance() for o in objs])
