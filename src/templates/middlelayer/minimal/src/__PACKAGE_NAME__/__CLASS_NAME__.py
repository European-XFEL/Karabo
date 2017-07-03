#############################################################################
# Author: __EMAIL__
# Created on __DATE__
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo.middlelayer import Device, Slot, String


class __CLASS_NAME__(Device):
    greeting = String()

    @Slot()
    def hello(self):
        self.greeting = "Hello world!"
