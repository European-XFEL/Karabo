from karabo.common.sanity_check import validate_macro
from karabo.middlelayer import (
    AccessLevel, AccessMode, Device, Macro, Overwrite, String, isSet)


class MetaMacro(Device):
    """This is the device that starts macros.

    It doesn't actually get instantiated itself, but when it is started will
    execute the code it is given and start the macros therein."""
    project = String(
        displayedName="Project Name",
        description="The project name of the macro it lives in.",
        accessMode=AccessMode.INITONLY)

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
        Macro.klass = None
        not_valid = validate_macro(self.code)
        if not_valid:
            msg = ("The macro cannot be started.\n"
                   "{}").format("\n".join([issue for issue in not_valid]))
            raise TypeError(msg)

        try:
            code = compile(self.code, self.module, "exec")
            exec(code, {})
            # Get the macro top level class
            self.klass = Macro.klass
        finally:
            Macro.klass = None

    def startInstance(self, server=None, broadcast=True):
        # this does not call super, as we don't want to run MetaMacro itself,
        # but only the macros in the supplied code
        deviceId = self.deviceId.value
        klass = self.klass
        parameters = {
            "_deviceId_":  f"{deviceId}-{klass.__name__}",
            "_serverId_": self.serverId.value,
            "hostName": self.hostName.value,
            "uuid": self.uuid.value,
            "module": self.module.value,
            "visibility": self.visibility.value,
        }
        if isSet(self.project):
            parameters["project"] = self.project.value
        macro = klass(parameters)
        macro.store_code(self.code.value)
        return macro.startInstance(server, broadcast=broadcast)
