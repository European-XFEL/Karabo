# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.bound import KARABO_CLASSINFO, PythonDevice


@KARABO_CLASSINFO("StuckLoggerDevice", "2.0")
class StuckLoggerDevice(PythonDevice):
    def slotLoggerContent(self, info):
        # bad behaviour. Implement a device that will never reply
        self._ss.createAsyncReply()
