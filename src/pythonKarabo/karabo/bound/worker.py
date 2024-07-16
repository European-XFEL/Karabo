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
import threading
import traceback
from collections import deque


class Worker(threading.Thread):

    def __init__(self, callback=None, timeout=-1, repetition=-1, daemon=True):
        """Constructs the Worker thread, that is by default a daemon thread.

           Please note that daemon threads may not release resources
           properly when stopped abruptly.
        """
        threading.Thread.__init__(self, daemon=daemon)
        self.callback = callback
        self.onError = None
        self.onExit = None
        self.timeout = timeout
        self.repetition = repetition
        self.running = False
        self.aborted = False
        self.suspended = False
        self.counter = -1
        self.cv = threading.Condition()  # cv = condition variable
        self.dq = deque()

    def set(self, callback, timeout=-1, repetition=-1):
        self.callback = callback
        self.timeout = timeout
        self.repetition = repetition

    def setTimeout(self, timeout=-1):
        self.timeout = timeout

    def setRepetition(self, repetition=-1):
        self.repetition = repetition

    def setErrorHandler(self, handler):
        self.onError = handler
        return self

    def setExitHandler(self, handler):
        self.onExit = handler
        return self

    def is_running(self):
        return self.running

    def push(self, o):
        if self.running:
            with self.cv:
                self.dq.append(o)
                self.cv.notify()

    def isRepetitionCounterExpired(self):
        return self.counter == 0

    def run(self):
        self.running = True
        self.aborted = False
        self.suspended = False
        self.counter = self.repetition
        try:
            if not callable(self.callback):
                raise ValueError("No callback is registered in Worker")
            while not self.aborted:
                t = None
                if self.counter == 0:
                    if callable(self.onExit):
                        self.onExit()
                    break
                if not self.running:
                    break
                if self.suspended:
                    with self.cv:
                        while self.suspended:
                            self.cv.wait()
                        if self.aborted or not self.running:
                            break
                    continue
                if self.timeout < 0:
                    with self.cv:
                        while len(self.dq) == 0:
                            self.cv.wait()
                        if not self.suspended:
                            t = self.dq.popleft()
                elif self.timeout > 0:
                    with self.cv:
                        if len(self.dq) == 0:
                            # self.timeout in milliseconds
                            self.cv.wait(float(self.timeout) / 1000)
                        if len(self.dq) != 0 and not self.suspended:
                            t = self.dq.popleft()
                else:
                    with self.cv:
                        if len(self.dq) != 0 and not self.suspended:
                            t = self.dq.popleft()
                if self.suspended:
                    continue
                if t is not None:
                    if self.stopCondition(t):
                        if callable(self.onExit):
                            self.onExit()
                        break
                if self.counter > 0:
                    self.counter -= 1
                if self.running:
                    self.callback()
        except Exception:
            if callable(self.onError):
                self.onError(traceback.format_exc())
            else:
                traceback.print_exc()

        if self.running:
            self.running = False

    def stopCondition(self, obj):
        return False

    def start(self):
        if not self.running:
            self.suspended = False
            super().start()
        if self.suspended:
            with self.cv:
                self.suspended = False
                self.cv.notify()
        return self

    def stop(self):
        if self.running:
            with self.cv:
                self.running = False
                self.suspended = False
                self.cv.notify()
        return self

    def abort(self):
        self.aborted = True
        self.running = False
        if self.suspended:
            with self.cv:
                self.suspended = False
                self.cv.notify()
        if len(self.dq) != 0:
            with self.cv:
                self.dq.clear()
        return self

    def pause(self):
        if not self.suspended:
            with self.cv:
                self.suspended = True
                self.cv.notify()


class QueueWorker(Worker):

    def __init__(self, callback):
        super().__init__(self.onWork)
        self.handler = callback
        self.msg = None

    def onWork(self):
        self.handler(self.msg)
        self.msg = None

    def stopCondition(self, msg):
        if "stop" in msg:
            return True
        self.msg = msg
        return False
