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

from karabo.bound import KARABO_CLASSINFO, PythonDevice

from ._version import version as deviceVersion


@KARABO_CLASSINFO("__CLASS_NAME__", deviceVersion)
class __CLASS_NAME__(PythonDevice):

    @staticmethod
    def expectedParameters(expected):
        """ This static method is needed as a part of the factory/configuration
        system.
        @param expected - Will contain the description of the device's expected
        parameters.
        NOTE: parenthesis () are used for allowing to switch off interpreter
        indentation rule.
        """
        (
        )

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super().__init__(configuration)
        # Define the first function to be called after the constructor has
        # finished
        self.registerInitialFunction(self.initialization)

        # If you need methods that can be callable from another device or GUI
        # you may register them here:
        # self.KARABO_SLOT(self.myslot1)
        # self.KARABO_SLOT(myslot2)
        # ...
        # It works for both methods (normal case) and standalone functions.
        # See documentation of KARABO_SLOT for more details.

        # Initialize your member variables here...
        # self._privateData = None

    def initialization(self):
        """
        This method will be called after the constructor.
        """
        # Register methods to process pipeline data.
        # Here 'def onData(self, data, meta)' is registered
        # for InputChannel "input" as defined in expectedParameters.
        # self.KARABO_ON_DATA("input", self.onData)
