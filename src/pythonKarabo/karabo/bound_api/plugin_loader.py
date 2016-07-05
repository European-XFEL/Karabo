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
        e.assignmentOptional().defaultValue("karabo.bound_device")
        e.expertAccess().commit()

        e = STRING_ELEMENT(expected).key("pluginNames")
        e.displayedName("Devices to Load")
        e.description("Comma separated list of class names of devices which should be loaded")
        e.assignmentOptional().defaultValue("")
        e.expertAccess()
        e.commit()

    def __init__(self, config):
        self._entrypoints = []
        if "pluginNamespace" in config:
            self.pluginNamespace = config["pluginNamespace"]
        else:
            msg = "A namespace must be defined for plugins to load."
            raise ValueError(msg)

        if "pluginDirectory" in config:
            self.pluginDirectory = op.abspath(config["pluginDirectory"])
        else:
            msg = "A directory must be defined for plugins to load."
            raise ValueError(msg)
        sys.path.append(self.pluginDirectory)

        self._filter_entrypoints = ()
        pluginNames = config["pluginNames"]
        if len(pluginNames) > 0:
            filt = [n for n in pluginNames.split(",") if len(n) > 0]
            self._filter_entrypoints = tuple(filt)

    def getPlugin(self, name):
        for ep in self._entrypoints:
            if ep.name == name:
                return ep
        else:
            raise RuntimeError("Plugin {} not found!".format(name))

    def update(self):
        ws = WorkingSet()
        epoints = list(ws.iter_entry_points(self.pluginNamespace))
        if len(self._filter_entrypoints) > 0:
            epoints = [ep for ep in epoints
                       if ep.name in self._filter_entrypoints]
        self._entrypoints = epoints
        return self._entrypoints
