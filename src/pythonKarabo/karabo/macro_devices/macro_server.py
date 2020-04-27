import sys

from karabo.middlelayer_api.device_server import MiddleLayerDeviceServer
from karabo.middlelayer import AccessLevel, Overwrite


class MacroServer(MiddleLayerDeviceServer):
    serverId = Overwrite(
        defaultValue="karabo/macroServer")
    pluginNamespace = Overwrite(
        defaultValue="karabo.macro_device")
    visibility = Overwrite(
        # when overwriting a descriptor with an enum attribute
        # one must always specify the options
        options=[l for l in AccessLevel],
        defaultValue=AccessLevel.ADMIN)

    def _initInfo(self):
        info = super(MacroServer, self)._initInfo()
        info["lang"] = "macro"
        return info


if __name__ == '__main__':
    MacroServer.main(sys.argv)
