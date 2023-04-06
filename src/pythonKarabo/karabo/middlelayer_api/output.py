""" This module redirects output """

from asyncio import get_event_loop


# XXX: This class should probably inherit from io.IOBase
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
