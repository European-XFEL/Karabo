"""Generalized interface to AGIPD
"""
from asyncio import wait_for

from .middlelayer import State, synchronize, waitUntil
from .genericproxy import Sensible


class AgipdAsSensible(Sensible):
    """Generalized interface to the AGIPD detector"""
    generalizes = ('AgipdComposite')
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
