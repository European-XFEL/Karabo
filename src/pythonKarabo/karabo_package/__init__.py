import sys
import os.path

__all__ = ["AccessLevel", "AccessMode", "Assignment", "String", "Integer",
           "waitUntil", "sleep", "Slot"]

if hasattr(sys, "karabo_api") and sys.karabo_api == 1:
    __path__ = [os.path.join(__path__[0], "legacy"), __path__[0]]
else:
    from karabo.device import Device
    __all__.append("Device")

import karabo.hash
from karabo.hashtypes import String, Int32 as Integer, Double as Float, Slot
from karabo.async import waitUntil
from asyncio import sleep
from karabo.enums import *

