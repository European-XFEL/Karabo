# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from enum import Enum, IntEnum


class Capabilities(IntEnum):
    PROVIDES_SCENES = 1
    PROVIDES_MACROS = 2
    PROVIDES_INTERFACES = 4
    # add future capabilities as bit maskable properties:
    # FUTURE_CAPABILITY = 8
    # SOME_OTHER_CAPABILITY = 16
    # ...


class Interfaces(IntEnum):
    Motor = 1
    MultiAxisMotor = 2
    Trigger = 4
    Camera = 8
    Processor = 16
    DeviceInstantiator = 32
    # add future interfaces as bit maskable properties:
    # FUTURE_INTERFACE = 64
    # SOME_OTHER_INTERFACE = 128
    # ...


class ServerFlags(IntEnum):
    Development = 1
    # add future serverFlags as bit maskable properties:
    # FUTURE_FLAG = 2
    # ...


class InstanceStatus(Enum):
    NONE = "none"
    OK = "ok"
    UNKNOWN = "unknown"
    ERROR = "error"
