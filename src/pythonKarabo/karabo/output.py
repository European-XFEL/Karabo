""" This module redirects output """

from asyncio import coroutine, get_event_loop

class KaraboStream:
    """ An output stream that redirects output to the karabo network """
    def __init__(self, base):
        self.base = base

    def write(self, data):
        try:
            get_event_loop().instance().printToConsole(data)
        except (AttributeError, AssertionError):
            self.base.write(data)

    def flush(self):
        get_event_loop().instance().update()
