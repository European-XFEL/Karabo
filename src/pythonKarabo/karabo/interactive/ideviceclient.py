# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from threading import Thread

from IPython import start_ipython

from karathon import EventLoop


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
