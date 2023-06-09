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
from .macro_api.alarms import showAlarms, showInterlocks
from .macro_api.device_interface import (
    listCameras, listDeviceInstantiators, listMotors, listMultiAxisMotors,
    listProcessors, listTriggers)


def _create_cli_submodule():
    """Create the namespace used by ikarabo."""
    from karabo.common.api import create_module

    # NOTE: This is the macro api part of the ikarabo namespace
    symbols = (
        showAlarms, showInterlocks, listCameras, listDeviceInstantiators,
        listMotors, listMultiAxisMotors, listProcessors, listTriggers
    )
    module = create_module('karabo.macros.cli', *symbols)
    module.__file__ = __file__  # looks nicer when repr(cli) is used
    return module


cli = _create_cli_submodule()
# Hide our implementation!
del _create_cli_submodule
