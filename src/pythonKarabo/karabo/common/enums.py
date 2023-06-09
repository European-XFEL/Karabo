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
