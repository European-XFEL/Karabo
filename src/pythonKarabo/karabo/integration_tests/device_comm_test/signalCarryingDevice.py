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

from karabo.bound import KARABO_CLASSINFO, PythonDevice


@KARABO_CLASSINFO("SignalDevice", "2.0")
class SignalDevice(PythonDevice):

    def __init__(self, configuration):
        super().__init__(configuration)

        self.signalSlotable.registerSignal("signal", int)

        self.KARABO_SLOT(self.slotEmitSignal)

    def slotEmitSignal(self, numInput):
        "Emit the signal with numInput (should be int!)"
        self.signalSlotable.emit("signal", numInput)
