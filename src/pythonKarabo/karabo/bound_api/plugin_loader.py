__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$May 8, 2013 12:55:50 PM$"

from pkg_resources import WorkingSet

from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karathon import STRING_ELEMENT


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonPluginLoader", "1.0")
class PluginLoader(object):

    @staticmethod
    def expectedParameters(expected):

        e = STRING_ELEMENT(expected).key("pluginNamespace")
        e.displayedName("Plugin Namespace")
        e.description("Namespace to search for plugins")
        e.assignmentOptional().defaultValue("karabo.bound_device")
        e.expertAccess().commit()

    def __init__(self, config):
        if "pluginNamespace" in config:
            self.pluginNamespace = config["pluginNamespace"]
        else:
            msg = "A namespace must be defined for plugins to load."
            raise ValueError(msg)

    def getPlugin(self, name):
        for ep in self._entrypoints:
            if ep.name == name:
                return ep
        else:
            raise RuntimeError("Plugin {} not found!".format(name))

    def update(self):
        ws = WorkingSet()
        epoints = list(ws.iter_entry_points(self.pluginNamespace))
        for ep in epoints:
            ep.load()
        return epoints
