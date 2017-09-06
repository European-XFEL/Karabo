"""Generalized interface to ePIX
"""
from .middlelayer import State, synchronize
from .genericproxy import Sensible


class EpixAsSensible(Sensible):
    """Generalized interface to the ePIX detector"""
    generalizes = ('ePixReceiver',)
    state_mapping = {State.STARTED: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()
