from asyncio import gather

from .device import Device
from .enums import AccessLevel, AccessMode
from .hash import String, Int32
from .macro import Macro
from ..common.macro_sanity_check import macro_sleep_check


class MetaMacro(Device):
    """This is the device that starts macros.

    It doesn't actually get instantiated itself, but when it is started will
    execute the code it is given and start the macros therein."""
    code = String(
        displayedName="Python Code",
        description="The code to be executed. It typically defines Macros",
        accessMode=AccessMode.INITONLY)
    module = String(
        displayedName="Name of Module",
        description="The name of the module within the project",
        accessMode=AccessMode.INITONLY)
    uuid = String(
        displayedName="Project DB UUID",
        description="The UUID for this macro",
        accessMode=AccessMode.INITONLY)
    visibility = Int32(enum=AccessLevel, defaultValue=AccessLevel.ADMIN)

    def __init__(self, config):
        super().__init__(config)
        Macro.subclasses = []

        sleeps = macro_sleep_check(self.code)
        if sleeps:
            lines = ",".join(str(idx) for idx in sleeps)
            msg = ("The usage of time.sleep on line(s) {} is forbidden."
                   " Use karabo.middlelayer.sleep instead".format(lines))
            raise ImportError(msg)

        try:
            code = compile(self.code, self.module, "exec")
            exec(code, {})
            self.classes = Macro.subclasses
        finally:
            Macro.subclasses = []

    def startInstance(self, server=None, broadcast=True):
        # this does not call super, as we don't want to run MetaMacro itself,
        # but only the macros in the supplied code
        p = {
            '_serverId_': self.serverId,
            'uuid': self.uuid,
            'module': self.module,
        }
        objs = []
        for c in self.classes:
            p["_deviceId_"] = "{}-{}".format(self.deviceId, c.__name__)
            objs.append(c(p))
        return gather(*(o.startInstance(server,
                                        broadcast=broadcast) for o in objs))
