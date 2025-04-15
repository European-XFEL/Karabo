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


@KARABO_CLASSINFO("StuckLoggerDevice", "2.0")
class StuckLoggerDevice(PythonDevice):
    def slotLoggerContent(self, info):
        # bad behaviour. Implement a device that will never reply
        self._sigslot.createAsyncReply()
