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

from karabo.middlelayer import Overwrite
from karabo.middlelayer.device_server import MiddleLayerDeviceServer
from karabo.middlelayer.output import KaraboStream


class MacroServer(MiddleLayerDeviceServer):
    serverId = Overwrite(
        defaultValue="karabo/macroServer")
    pluginNamespace = Overwrite(
        defaultValue="karabo.macro_device")

    def _initInfo(self):
        info = super()._initInfo()
        info["lang"] = "macro"
        return info

    async def _run(self, **kwargs):
        await super()._run(**kwargs)
        sys.stdout = KaraboStream(sys.stdout)
        sys.stderr = KaraboStream(sys.stderr)


if __name__ == '__main__':
    MacroServer.main(sys.argv)
