__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 8, 2013 12:55:50 PM$"

import sys

from pkg_resources import WorkingSet

from .enums import Assignment, AccessLevel
from .hash import String
from .schema import Configurable


class PluginLoader(Configurable):
    pluginDirectory = String(
        displayedName="Plugin Directory",
        description="Directory to search for plugins",
        assignment=Assignment.OPTIONAL, defaultValue="plugins",
        displayType="directory", requiredAccessLevel=AccessLevel.EXPERT)

    pluginNamespace = String(
        displayedName="Plugin Namespace",
        description="Namespace to search for plugins",
        assignment=Assignment.OPTIONAL,
        defaultValue="karabo.python_device.api_2",
        requiredAccessLevel=AccessLevel.EXPERT)

    def __init__(self, input):
        super(PluginLoader, self).__init__(input)
        self._entrypoints = []
        sys.path.append(self.pluginDirectory)

    def getPlugin(self, name):
        for ep in self._entrypoints:
            if ep.name == name:
                return ep
        else:
            raise RuntimeError("Plugin {} not found!".format(name))

    def update(self):
        ws = WorkingSet()
        self._entrypoints = list(ws.iter_entry_points(self.pluginNamespace))
        return self._entrypoints
