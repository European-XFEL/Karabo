from asyncio import gather

from karabo.common.macro_sanity_check import validate_macro
from karabo.middlelayer import (
    AccessLevel, AccessMode, Device, Macro, String, Overwrite)


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
    visibility = Overwrite(
        options=[level for level in AccessLevel],
        defaultValue=AccessLevel.ADMIN)

    def __init__(self, config):
        super().__init__(config)
        Macro.subclasses = []

        not_valid = validate_macro(self.code)
        if not_valid:
            msg = ("The macro cannot be started.\n"
                   "{}").format("\n".join([issue for issue in not_valid]))
            raise TypeError(msg)

        try:
            code = compile(self.code, self.module, "exec")
            exec(code, {})
            self.classes = Macro.subclasses
        finally:
            Macro.subclasses = []

    def startInstance(self, server=None, broadcast=True):
        # this does not call super, as we don't want to run MetaMacro itself,
        # but only the macros in the supplied code
        parameters = {
            '_serverId_': self.serverId.value,
            'hostName': self.hostName.value,
            'uuid': self.uuid.value,
            'module': self.module.value,
            'visibility': self.visibility.value,
        }
        objs = []
        for klass in self.classes:
            # The extraction of the value attribute is not strictly needed
            # since we format `self.deviceId` in a string.
            # By using the value attribute we isolate this code from the
            # representation of a `String`
            deviceId = self.deviceId.value
            parameters["_deviceId_"] = f"{deviceId}-{klass.__name__}"
            objs.append(klass(parameters))
        return gather(*(o.startInstance(server,
                                        broadcast=broadcast) for o in objs))
