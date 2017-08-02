"""Generalized interface to AGIPD
"""
from asyncio import wait_for
from .genericproxy import Sensible
from karabo.middlelayer import State, waitUntil
from karabo.middlelayer_api.eventloop import synchronize


class AgipdAsSensible(Sensible):
    """Generalized interface to the AGIPD detector"""
    generalizes = ('AgipdControl')
    state_mapping = {State.STARTED: State.ACQUIRING}
    connection_timeout = 10.0

    @synchronize
    def acquire(self):
        """Start acquisition"""
        if self._proxy.state == State.UNKNOWN:
            yield from self._proxy.connect()
            yield from wait_for(
                waitUntil(lambda: self._proxy.state == State.ON),
                self.connection_timeout)

        yield from self._proxy.start()

    @synchronize
    def stop(self):
        """Stop acquisition"""
        yield from self._proxy.stop()
