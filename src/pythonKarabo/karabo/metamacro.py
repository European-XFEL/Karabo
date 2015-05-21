from asyncio import coroutine, gather

from karabo.enums import AccessMode
from karabo.python_device import Device
from karabo.hashtypes import String
from karabo.macro import Macro


class MetaMacro(Device):
    """This is the device that starts macros.

    It doesn't actually get instantiated itself, but when it is started will
    execute the code it is given and start the macros therein."""
    code = String(
        displayedName="Python Code",
        description="The code to be executed. It typically defines Macros",
        accessMode=AccessMode.INITONLY)
    project = String(
        displayedName="Containing Project",
        description="The project this code is contained in",
        accessMode=AccessMode.INITONLY)
    module = String(
        displayedName="Name of Module",
        description="The name of the module within the project",
        accessMode=AccessMode.INITONLY)

    def __init__(self, config):
        super().__init__(config)
        Macro.subclasses = []
        try:
            code = compile(self.code, self.module, "exec")
            exec(code, {})
            self.classes = Macro.subclasses
        finally:
            Macro.subclasses = []

    def startInstance(self, server=None):
        # this does not call super, as we don't want to run MetaMacro itself,
        # but only the macros in the supplied code
        p = dict(_serverId_=self.serverId, project=self.project,
                 module=self.module)
        objs = []
        for c in self.classes:
            p["_deviceId_"] = "{}-{}".format(self.deviceId, c.__name__)
            objs.append(c(p))
        return gather(*(o.startInstance(server) for o in objs))
