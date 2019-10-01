# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import sys
import time
import threading
 
from karabo.bound import (
        PythonDevice, KARABO_CLASSINFO, State, UINT64_ELEMENT,
)

@KARABO_CLASSINFO("TinyBoundDevice", "2.0")
class TinyBoundDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        (
        UINT64_ELEMENT(expected).key("updateId")
                .displayedName("Update ID")
                .description("Number of readings from NICs")
                .readOnly().initialValue(0)
                .commit()
                ,
        )

    def __init__(self, configuration):
        self.running = False
        self.pollingThread = None
        self.registerInitialFunction(self.initialization)

    def initialization(self):
        self.updateState(State.INIT)
        self.set("updateId", 0)
        time.sleep(1)
        self.startPolling()

    #def preDestruction(self):
    #    self.running = False;

    def startPolling(self):
        self.updateState(State.MONITORING)
        self.running = True
        if self.pollingThread is None:
            self.pollingThread = threading.Thread(target=self.polling)
            self.pollingThread.start()

    def polling(self):
        while self.running:
            self.set("updateId", self.get("updateId") + 1)
            time.sleep(1)

# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
