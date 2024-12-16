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
""" This module redirects output """

from asyncio import get_event_loop


class KaraboStream:
    """ An output stream that redirects output to the karabo network """

    def __init__(self, base):
        self.base = base

    def isatty(self):
        return False

    def write(self, data):
        try:
            instance = get_event_loop().instance()
            instance._ss.loop.call_soon_threadsafe(
                instance.printToConsole, data)
        except BaseException:
            self.base.write(data)

    def flush(self):
        try:
            get_event_loop().instance().update()
        except BaseException:
            # RuntimeError can appear when no local loop has been set anymore.
            # We simply flush to keep going.
            self.base.flush()

    @property
    def fileno(self):
        return self.base.fileno
