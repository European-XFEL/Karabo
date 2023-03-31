#############################################################################
# Author: __EMAIL__
#
# Created on __DATE__
# from template '__TEMPLATE_ID__' of Karabo __KARABO_VERSION__
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################

from karabo.middlelayer import Device, Slot, String

from ._version import version as deviceVersion


class __CLASS_NAME__(Device):
    __version__ = deviceVersion

    greeting = String()

    @Slot()
    async def hello(self):
        self.greeting = "Hello world!"

    def __init__(self, configuration):
        super().__init__(configuration)

    async def onInitialization(self):
        """ This method will be called when the device starts.

            Define your actions to be executed after instantiation.
        """
