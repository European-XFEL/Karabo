# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
# flake8: noqa

""" This is a broken device, it only serves test purposes
"""

import this_module_does_not_exist

from karabo.bound import KARABO_CLASSINFO, PythonDevice


@KARABO_CLASSINFO("TestDevice", "1.5")
class TestDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        pass
