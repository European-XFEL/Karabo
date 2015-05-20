#!/usr/bin/env python3

__author__="__EMAIL__"
__date__ ="__DATE__"
__copyright__="Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved."

from karabo.configurator import Configurator
from __CLASS_NAME__ import *

if __name__ == "__main__":
    device = Configurator(PythonDevice).create("__CLASS_NAME__", Hash("Logger.priority", "DEBUG", "deviceId", "__CLASS_NAME__Main_0"))
    device.run()
