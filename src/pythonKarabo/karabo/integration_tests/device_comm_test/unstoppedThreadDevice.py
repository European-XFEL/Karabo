# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import os
import signal
import sys
import threading
import time

from karabo.bound import (
    KARABO_CLASSINFO, SLOT_ELEMENT, UINT64_ELEMENT, PythonDevice, State)


@KARABO_CLASSINFO("UnstoppedThreadDevice", "2.0")
class UnstoppedThreadDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        (
        UINT64_ELEMENT(expected).key("updateId")
                .displayedName("Update ID")
                .description("Number of readings from NICs")
                .readOnly().initialValue(0)
                .commit()
                ,

        SLOT_ELEMENT(expected).key("slotPutToComa")
        .displayedName("Put to coma")
        .description("Put device into coma, i.e. linux process alive, but "
                     "dead Karabo-wise")
        .commit()
        )

    def __init__(self, configuration):
        super().__init__(configuration)
        self.running = False
        self.pollingThread = None
        self.KARABO_SLOT(self.slotPutToComa)
        self.registerInitialFunction(self.initialization)

    def initialization(self):
        self.updateState(State.INIT)
        self.set("updateId", 0)
        time.sleep(1)
        self.startPolling()

    # A well behaving device would have to stop the polling thread
    # in preDestruction to allow clean shutdown. Here we make it
    # ill-behaving (by commenting) on purpose to test that the server
    # will kill it anyway after some seconds.

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

    def slotPutToComa(self):
        self.log.WARN("Committing silent suicide, keeping process alive.")

        # This will trigger the central event-loop to finish
        # (in about 1 s, see C++'s Eventlopp::work()).
        # But the thread is still keeping the device process alive!
        os.kill(os.getpid(), signal.SIGTERM)
