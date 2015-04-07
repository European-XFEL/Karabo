#!/usr/bin/env python

__author__="__EMAIL__"
__date__ ="__DATE__"
__copyright__="Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved."

import time
import sys
import threading
from karabo.device import *
from karabo.ok_error_fsm import OkErrorFsm   # <-- choose custom FSM if any


@KARABO_CLASSINFO("__CLASS_NAME__", "1.0")
class __CLASS_NAME__(PythonDevice, OkErrorFsm):

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super(__CLASS_NAME__,self).__init__(configuration)
        self.workerThread = None
        self.keepWorking = False
        
    @staticmethod
    def expectedParameters(expected):
        '''Description of device parameters statically known'''
        (
        INT32_ELEMENT(expected)
                .key("startcount")
                .displayedName("Start Count")
                .description("Start count.")
                .assignmentOptional().defaultValue(50)
                .reconfigurable()
                .commit()
                ,
        INT32_ELEMENT(expected)
                .key("countdown")
                .displayedName("Countdown")
                .description("Counter showing the number of yet not written dots.")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .commit()
                ,
        STRING_ELEMENT(expected)
                .key("karabo")
                .displayedName("Karabo")
                .description("Counter showing the number of yet not written dots.")
                .assignmentOptional().defaultValue("")
                .reconfigurable()
                .commit()
                ,
        )

    ##############################################
    #   Implementation of State Machine methods  #
    ##############################################

    def okStateOnEntry(self):
        '''Start thread with user business logic.'''
        #raise ValueError,"Introduced artificial exception :)"
        self.workerThread = threading.Thread(target=self.mainWork, name="MainWork")
        self.workerThread.start()
        self.log.INFO("Device running...")
        
    def okStateOnExit(self):
        '''Stop worker thread.'''
        try:
            self.keepWorking = False
            if self.workerThread is not None and self.workerThread.is_alive():
                self.workerThread.join()
        except Exception, e:
            self.log.ERROR("Exception while joining worker thread", str(e))
        else:
            self.log.INFO("Device stopped...")
        
    def mainWork(self):
        '''Put here business logic.'''
        try:
            self["karabo"] = "Attention, ple-e-e-ase!)"
            self["countdown"] = self["startcount"]
            self.keepWorking = True
            while self.keepWorking:
                if self["countdown"] == 0:
                    raise ValueError,"Exception raised intentionally! :)"
                sys.stdout.write('.')
                sys.stdout.flush()
                self["countdown"] = self["countdown"] - 1  # set into device and report changes to GUI
                time.sleep(0.1)
        except Exception, e:
            self.execute("errorFound", "Exception", str(e))  # goto Error state
            self["karabo"] = "B-O-O-O-M-M-S-S!!! :("
            self.log.INFO("Exit mainWork()...")
            

    # Put here more state machibe actioins if needed... . See FSM API
   
# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
