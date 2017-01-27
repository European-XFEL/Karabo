"""
Author: __EMAIL__
Creation date: __DATE__
Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
"""

from karabo.middlelayer import Device, Slot, String


class __CLASS_NAME__(Device):
    greeting = String()

    @Slot()
    def hello(self):
        self.greeting = "Hello world!"
