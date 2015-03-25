# this script is loaded at the beginning of a cli session

import karabo
karabo.api_version = 2

from asyncio import set_event_loop

from karabo.device_client import (
    getDevice, waitUntil, waitUntilNew, setNoWait, executeNoWait)
from karabo.eventloop import NoEventLoop
from karabo.macro import Macro

m = Macro()
set_event_loop(NoEventLoop(m))

__all__ = ["getDevice", "waitUntil", "waitUntilNew", "setNoWait",
           "executeNoWait"]
