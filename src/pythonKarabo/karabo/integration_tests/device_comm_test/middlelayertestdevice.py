from asyncio import coroutine

from karabo.middlelayer import Device, getDevice, Int32, sleep


class MiddleLayerTestDevice(Device):
    something = Int32(defaultValue=333)
    counter = Int32(defaultValue=-1)

    @coroutine
    def onInitialization(self):
        self.myself = yield from getDevice("other")
        self.myself.something = 222

    @coroutine
    def onDestruction(self):
        with self.myself:
            self.myself.something = 111
            yield from sleep(0.02)


class SomeDevice(Device):
    pass
