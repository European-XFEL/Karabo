from asyncio import async, coroutine
from itertools import count

from karabo.middlelayer import Device, getDevice, Int32, sleep, Slot


class MiddleLayerTestDevice(Device):
    something = Int32(defaultValue=333)
    counter = Int32(defaultValue=-1)

    @coroutine
    def do_count(self):
        for i in count():
            self.counter = i
            yield from sleep(0.1)

    @Slot()
    @coroutine
    def count(self):
        async(self.do_count())

    @coroutine
    def onInitialization(self):
        self.myself = yield from getDevice("other")
        self.myself.something = 222

    @coroutine
    def onDestruction(self):
        with self.myself:
            self.myself.something = 111
            yield from sleep(0.02)
