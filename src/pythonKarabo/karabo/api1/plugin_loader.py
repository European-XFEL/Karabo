__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 8, 2013 12:55:50 PM$"

import os.path as op
import sys

from pkg_resources import WorkingSet

from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karathon import PATH_ELEMENT, STRING_ELEMENT


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonPluginLoader", "1.0")
class PluginLoader(object):

    @staticmethod
    def expectedParameters(expected):
        e = PATH_ELEMENT(expected).key("pluginDirectory")
        e.displayedName("Plugin Directory")
        e.description("Directory to search for plugins")
        e.isDirectory().assignmentOptional().defaultValue("plugins")
        e.expertAccess().commit()

        e = STRING_ELEMENT(expected).key("pluginNamespace")
        e.displayedName("Plugin Namespace")
        e.description("Namespace to search for plugins")
        e.assignmentOptional().defaultValue("karabo.python_device.api_1")
        e.expertAccess().commit()

    def __init__(self, input):
        self._entrypoints = []
        if "pluginNamespace" in input:
            self.pluginNamespace = input["pluginNamespace"]
        else:
            msg = "A namespace must be defined for plugins to load."
            raise ValueError(msg)
        if "pluginDirectory" in input:
            self.pluginDirectory = op.abspath(input["pluginDirectory"])
        else:
            msg = "A directory must be defined for plugins to load."
            raise ValueError(msg)
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
