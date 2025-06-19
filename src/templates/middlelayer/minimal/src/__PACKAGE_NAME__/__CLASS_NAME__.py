#############################################################################
# Author: __EMAIL__
#
# Created on __DATE__
# from template '__TEMPLATE_ID__' of Karabo __KARABO_VERSION__
#
# This file is intended to be used together with Karabo:
#
# http://www.karabo.eu
#
# IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
# Karabo itself is licensed under the terms of the MPL 2.0 license.
#############################################################################

from karabo.middlelayer import Device

from ._version import version as deviceVersion


class __CLASS_NAME__(Device):
    __version__ = deviceVersion

    def __init__(self, configuration):
        super().__init__(configuration)

    async def onInitialization(self):
        """ This method will be called when the device starts.

            Define your actions to be executed after instantiation.
        """
