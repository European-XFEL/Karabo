from asyncio import wait_for

from karabo.middlelayer import Bool, Device, State, String, allCompleted, isSet

SEARCH_WAIT = 0.5


class WaiterDevice(Device):

    waiterId = String()

    initialDiscover = Bool(
        defaultValue=False)

    hasCancellation = Bool(
        defaultValue=True)

    hasDevice = Bool(
        defaultValue=False)

    async def onInitialization(self):
        assert isSet(self.waiterId)
        device = self.getLocalDevice(self.waiterId)
        self.initialDiscover = True if device else False
        self.state = State.CHANGING
        futs = {"dev": self.waitLocalDevice(self.waiterId.value),
                "cancel": wait_for(self.waitLocalDevice(self.waiterId.value),
                                   timeout=0.001)}
        done, _, error = await allCompleted(**futs)
        self.hasDevice = True if done.get("dev") else False
        self.hasCancellation = True if error.get("cancel") else False
        self.state = State.ON
