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
from threading import Thread

from IPython import start_ipython

from karabo.bound import EventLoop


def main():
    # We need the (C++) event loop in the background:
    loopThread = Thread(target=EventLoop.work)
    loopThread.start()

    code_to_run = 'from karabo.interactive.deviceClient import *'
    start_ipython(code_to_run=code_to_run, force_interact=True,
                  display_banner=False)

    EventLoop.stop()
    loopThread.join()


if __name__ == '__main__':
    main()
