import sys
import os.path

__all__ = ["AccessLevel", "AccessMode", "Assignment", "String", "Integer",
           "waitUntil", "sleep", "Slot", "Bool"]

if hasattr(sys, "karabo_api") and sys.karabo_api == 2:
    from karabo.device import Device
    __all__.append("Device")
    import karabo.hash
    from karabo.hashtypes import (String, Int32 as Integer, Double as Float,
                                  Slot, Bool)
    from karabo.async import waitUntil
    from asyncio import sleep
    from karabo.enums import *
else:
    __path__ = [os.path.join(__path__[0], "legacy"), __path__[0]]


