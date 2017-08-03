"""Generalized interface to LPD
"""
from asyncio import wait_for

from karabo.middlelayer import State, waitUntil
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Sensible


class LpdAsSensible(Sensible):
    """Generalized interface to the LPD detector"""
    generalizes = ('LpdComposite')
    state_mapping = {State.STARTED: State.ACQUIRING}
    connection_timeout = 10.0

    @synchronize
    def acquire(self):
        """Start acquisition"""
        if self._proxy.state == State.UNKNOWN:
            yield from self._proxy.connectFEM()
            yield from wait_for(
                waitUntil(lambda: self._proxy.state == State.ON),
                self.connection_timeout)

        yield from self._proxy.startDAQ()

    @synchronize
    def stop(self):
        """Stop acquisition"""
        yield from self._proxy.stopDAQ()
