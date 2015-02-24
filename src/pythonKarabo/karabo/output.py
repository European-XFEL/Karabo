""" This module redirects output """

from asyncio import coroutine, Task
import threading

threadData = threading.local()

class KaraboStream:
    """ An output stream that redirects output to the karabo network """
    def __init__(self, base):
        self.base = base

    def write(self, data):
        try:
            task = Task.current_task()
            task.instance.printToConsole(data)
        except (AttributeError, AssertionError):
            try:
                instance = threadData.instance
                func = instance.printToConsole
                @coroutine
                def print():
                    func(data)
                instance.async(print())
            except AttributeError:
                self.base.write(data)



