# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 8, 2013 12:55:50 PM$"

import os
import sys
import re
from karabo_decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from libkarathon import PATH_ELEMENT


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonPluginLoader", "1.0")
class PluginLoader(object):

    @staticmethod
    def expectedParameters(expected):
        
        e = PATH_ELEMENT(expected).key("pluginDirectory")
        e.displayedName("Plugin Directory").description("Directory to search for plugins")
        e.assignmentOptional().defaultValue("plugins")
        e.isDirectory().advanced().commit()
        
    def __init__(self, input):
        if "pluginDirectory" in input:
            self.plugins = input["pluginDirectory"]
        else:
            self.plugins = os.environ['PWD'] + "/plugins"
        sys.path.append(self.plugins)
        self.pattern = re.compile(r'.py')

    def getPluginDirectory(self):
        return self.plugins

    def update(self):
        matches = filter(lambda m: not m is None, [self.pattern.search(x) for x in os.listdir(self.plugins)])
        modules = list()
        for m in matches:
            name = m.string[0:m.start()]              # module name
            path = self.plugins + "/" + m.string      # path to module source
            if path.endswith(".pyc"):                 # skip compiled version
                continue
            modules.append((name, path,))
        return modules

