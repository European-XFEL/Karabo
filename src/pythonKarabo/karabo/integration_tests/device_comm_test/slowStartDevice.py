# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
