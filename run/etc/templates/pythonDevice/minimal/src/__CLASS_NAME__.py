#!/usr/bin/env python

__author__="__EMAIL__"
__date__ ="__DATE__"
__copyright__="Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved."

import time
import sys
from karabo.device import *


@KARABO_CLASSINFO("__CLASS_NAME__", "1.3")
class __CLASS_NAME__(PythonDevice):

    @staticmethod
    def expectedParameters(expected):
        '''
        This static method is needed as a part of the factory/configuration system.
        @param expected Will contain the description of the device expected parameters.
        NOTE: parenthesis () are used for allowing to switch off interpreter indentation rule.
        '''
        (
        )

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super(__CLASS_NAME__,self).__init__(configuration)
        # Define the first function to be called after the constructor has finished
        self.registerInitialFunction(self.initialization)
        # Initialize here your member variables...

    def initialization(self):
        '''
        This method will be called after the constructor.
        if you need methods that can be callable from another device or GUI
        you may register them here:
        self.KARABO_SLOT(self.myslot1)
        self.KARABO_SLOT(self.myslot2)
        ...
        Corresponding methods (myslot1, myslot2, ...) should be defined in this class
        '''
        
    # Put here your slots 


# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
