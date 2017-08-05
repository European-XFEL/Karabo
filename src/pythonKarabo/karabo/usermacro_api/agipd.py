"""Generalized interface to AGIPD
"""
from asyncio import wait_for

from karabo.middlelayer import State, waitUntil
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Sensible


class AgipdAsSensible(Sensible):
    """Generalized interface to the AGIPD detector"""
    generalizes = ('AgipdControl')
    state_mapping = {State.STARTED: State.ACQUIRING}

    @synchronize
    def prepare(self):
        """Get ready for acquisition"""
        if self._proxy.state == State.UNKNOWN:
            yield from self._proxy.connect()
            yield from wait_for(
                waitUntil(lambda: self._proxy.state == State.ON),
                self.preparation_timeout)

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()
