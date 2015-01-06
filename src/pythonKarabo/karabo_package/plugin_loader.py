# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 8, 2013 12:55:50 PM$"

import os
import sys
from subprocess import call
import re
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.karathon import PATH_ELEMENT


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonPluginLoader", "1.0")
class PluginLoader(object):

    @staticmethod
    def expectedParameters(expected):
        
        e = PATH_ELEMENT(expected).key("pluginDirectory")
        e.displayedName("Plugin Directory").description("Directory to search for plugins")
        e.assignmentOptional().defaultValue("plugins")
        e.isDirectory().expertAccess().commit()

    def __init__(self, input):
        if "pluginDirectory" in input:
            self.plugins = input["pluginDirectory"]
        else:
            self.plugins = os.environ['PWD'] + "/plugins"
        sys.path.append(self.plugins)

    def getPluginDirectory(self):
        return self.plugins

    def update(self):
        return [n[:-3] for n in os.listdir(self.plugins) if n.endswith(".py")]
