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
import time

from karabo.bound import INT32_ELEMENT, KARABO_CLASSINFO, PythonDevice


@KARABO_CLASSINFO("SlowStartDevice", "2.0")
class SlowStartDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        (
            INT32_ELEMENT(expected).key("initSleep")
            .assignmentOptional().defaultValue(5)
            .commit(),
        )

    def __init__(self, configuration):
        super().__init__(configuration)
        # This is bad - __init__ should no block. Do here for test purpose.
        # Any blocking should move in a method registered for initialisation.
        time.sleep(configuration["initSleep"])
