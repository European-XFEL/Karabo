"""Generalzed interface to LPD
"""
from asyncio import wait_for
from .genericproxy import Sensible
from karabo.middlelayer import State, waitUntil
from karabo.middlelayer_api.eventloop import synchronize


class LpdAsSensible(Sensible):
    """Generalized interface to the LPD detector"""
    generalizes = ['LpdComposite']
    state_mapping = {State.STARTED: State.ACQUIRING}
    connection_timeout = 10.0

    @property
    def state(self):
        """Get state"""
        return self.state_mapping.get(self._proxy.state, self._proxy.state)

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
