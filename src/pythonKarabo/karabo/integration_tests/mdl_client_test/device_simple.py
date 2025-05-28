from karabo.middlelayer import DeviceClientBase, Hash, State, slot


class SimpleTopology(DeviceClientBase):

    @slot
    def getTopology(self, info: Hash) -> Hash:
        return self.systemTopology

    async def onInitialization(self):
        self.state = State.ON
