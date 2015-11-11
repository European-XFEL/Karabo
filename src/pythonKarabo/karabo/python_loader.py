__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 8, 2013 12:55:50 PM$"

import os
import sys

from karabo.enums import Assignment, AccessLevel
from karabo.hash import String
from karabo.schema import Configurable


class PluginLoader(Configurable):
    pluginDirectory = String(
        displayedName="Plugin Directory",
        description="Directory to search for plugins",
        assignment=Assignment.OPTIONAL, defaultValue="plugins",
        displayType="directory", requiredAccessLevel=AccessLevel.EXPERT)

    def __init__(self, input):
        super(PluginLoader, self).__init__(input)
        sys.path.append(self.pluginDirectory)

    def update(self):
        return [n[:-3] for n in os.listdir(self.pluginDirectory)
                if n.endswith(".py")]
