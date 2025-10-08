# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from karabo.common.sanity_check import validate_macro
from karabo.middlelayer import AccessMode, Device, Macro, String, isSet


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

    def startInstance(self, server=None, broadcast=True, discover=True):
        # this does not call super, as we don't want to run MetaMacro itself,
        # but only the macros in the supplied code
        deviceId = self.deviceId.value
        klass = self.klass
        parameters = {
            "_deviceId_":  f"{deviceId}",
            "serverId": self.serverId.value,
            "hostName": self.hostName.value,
            "uuid": self.uuid.value,
            "module": self.module.value,
        }
        if isSet(self.project):
            parameters["project"] = self.project.value
        macro = klass(parameters)
        macro.store_code(self.code.value)
        return macro.startInstance(
            server, broadcast=broadcast, discover=discover)
