from karabo.api2.enums import (
    AccessLevel, AccessMode, Assignment, MetricPrefix, Unit)
from karabo.api2.hash import String, Int32 as Int, Slot, Double as Float, Bool
from karabo.api2.macro import Macro
from karabo.api2.device_client import (
    waitUntilNew, waitUntil, setWait, setNoWait, getDevice, executeNoWait,
    updateDevice, sleep)
