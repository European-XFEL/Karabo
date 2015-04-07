#!/usr/bin/env python

__author__="__EMAIL__"
__date__ ="__DATE__"
__copyright__="Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved."

import time
from karabo.device import *
from karabo.start_stop_fsm import StartStopFsm

@KARABO_CLASSINFO("__CLASS_NAME__", "1.0")
class __CLASS_NAME__(PythonDevice, StartStopFsm):

    def __init__(self, configuration):
        # always call superclass constructor first!
        super(__CLASS_NAME__,self).__init__(configuration)
        
    @staticmethod
    def expectedParameters(expected):
        e = DOUBLE_ELEMENT(expected).key("targetSpeed")
        e.displayedName("Target Conveyor Speed")
        e.description("Configures the speed of the conveyor belt")
        e.unit(METER_PER_SECOND)
        e.assignmentOptional().defaultValue(0.8)
        e.reconfigurable()
        e.commit()

        e = DOUBLE_ELEMENT(expected).key("currentSpeed")
        e.displayedName("Current Conveyor Speed")
        e.description("Shows the current speed of the conveyor")
        e.readOnly()
        e.commit()

        e = BOOL_ELEMENT(expected)
        e.key("reverseDirection").displayedName("Reverse Direction")
        e.description("Reverses the direction of the conveyor band")
        e.assignmentOptional().defaultValue(False).reconfigurable()
        e.allowedStates("AllOkState.StoppedState")
        e.commit()

    ##############################################
    #   Implementation of State Machine methods  #
    ##############################################

    def initializationStateOnEntry(self):
        self.log.INFO("Connecting to conveyer hardware, setting up motors...")
        self.set("currentSpeed", 0.0)
        
    def startAction(self):
        # Retrieve current values from our own device-state
        tgtSpeed     = self.get("targetSpeed")
        currentSpeed = self.get("currentSpeed")
        
        # If we do not stand still here that is an error
        if currentSpeed > 0.0:
            raise ValueError,"Conveyer does not stand still at start-up"
        
        increase = tgtSpeed / 50.0
        
        # Simulate a slow ramping up of the conveyor
        for i in range(50):
            currentSpeed += increase
            self.set("currentSpeed", currentSpeed);
            time.sleep(0.05)
        # Be sure to finally run with targetSpeed
        self.set("currentSpeed", tgtSpeed)
        
    def stopAction(self):
        # Retrieve current value from our own device-state
        currentSpeed = self.get("currentSpeed")
        # Separate ramping into 50 steps
        decrease = currentSpeed / 50.0
        # Simulate a slow ramping down of the conveyor
        for i in range(50):
            currentSpeed -= decrease
            self.set("currentSpeed", currentSpeed)
            time.sleep(0.05)
        # Be sure to finally stand still
        self.set("currentSpeed", 0)
    
        
if __name__ == "__main__":
    launchPythonDevice()