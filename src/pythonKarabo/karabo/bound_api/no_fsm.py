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
# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

__author__ = "Sergey Esenov serguei.essenov@xfel.eu"
__date__ = "$Nov 26, 2014 3:18:24 PM$"

from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("NoFsm", "1.3")
class NoFsm:

    @staticmethod
    def expectedParameters(expected):
        pass

    def __init__(self, configuration):
        super().__init__()
        self.func = []

    def startFsm(self):
        """Start state machine"""
        # call initial function registered in the device constructor
        # on registration order
        for f in self.func:
            f()

    def registerInitialFunction(self, func):
        self.func.append(func)

    def stopFsm(self): pass

    def errorFound(self, userFriendly, detail):
        print(f"*** ERROR *** : {userFriendly} -- {detail}")

    def exceptionFound(self, userFriendlyMessage, detailedMessage):
        pass
