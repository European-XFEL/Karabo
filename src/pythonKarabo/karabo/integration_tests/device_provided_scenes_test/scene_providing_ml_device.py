from asyncio import coroutine

from karabo.common.states import State
from karabo.middlelayer import Device, VectorString


class SceneProvidingMLDevice(Device):
    availableScenes = VectorString()

    @coroutine
    def onInitialization(self):
        self.state = State.NORMAL


class NonSceneProvidingMLDevice(Device):

    @coroutine
    def onInitialization(self):
        self.state = State.NORMAL
