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
import sys

from karabo.middlelayer import AccessLevel, Overwrite
from karabo.middlelayer.device_server import MiddleLayerDeviceServer


class MacroServer(MiddleLayerDeviceServer):
    serverId = Overwrite(
        defaultValue="karabo/macroServer")
    pluginNamespace = Overwrite(
        defaultValue="karabo.macro_device")
    visibility = Overwrite(
        # when overwriting a descriptor with an enum attribute
        # one must always specify the options
        options=[level for level in AccessLevel],
        defaultValue=AccessLevel.ADMIN)

    def _initInfo(self):
        info = super()._initInfo()
        info["lang"] = "macro"
        return info


if __name__ == '__main__':
    MacroServer.main(sys.argv)
