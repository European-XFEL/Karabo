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
import traceback
from importlib.metadata import entry_points

from karabind import STRING_ELEMENT

from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS

DEFAULT_NAMESPACE = "karabo.bound_device"


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonPluginLoader", "1.0")
class PluginLoader:

    @staticmethod
    def expectedParameters(expected):

        e = STRING_ELEMENT(expected).key("pluginNamespace")
        e.displayedName("Plugin Namespace")
        e.description("Namespace to search for plugins")
        e.assignmentOptional().defaultValue(DEFAULT_NAMESPACE)
        e.expertAccess().commit()

    def __init__(self, config):
        self._entrypoints = []
        self.plugin_errors = {}

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
            raise RuntimeError(f"Plugin {name} not found!")

    def update(self):
        """Protected entry to update the entrypoints"""
        def load(ep):
            try:
                ep.load()
                return True
            except BaseException:
                self.plugin_errors[ep.name] = traceback.format_exc()
                return False

        self._entrypoints = [
            ep for ep in entry_points(group=self.pluginNamespace)
            if load(ep)]

        return self._entrypoints
