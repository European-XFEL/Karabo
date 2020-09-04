#############################################################################
# Author: __EMAIL__
# Created on __DATE__
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo.bound import PythonDevice, KARABO_CLASSINFO

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
        # Initialize your member variables here...

    def initialization(self):
        """ This method will be called after the constructor.

        If you need methods that can be callable from another device or GUI
        you may register them here:
        self.KARABO_SLOT(self.myslot1)
        self.KARABO_SLOT(self.myslot2)
        ...
        Corresponding methods (myslot1, myslot2, ...) should be defined in this
        class
        """
        # Define your slots here
