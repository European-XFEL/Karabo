"""Generalized interface to simulated LightBeam
"""
from .genericproxy import Sensible
from .middlelayer import State, synchronize


class LightBeamAsSensible(Sensible):
    """Generalized interface to the LightBeam"""
    generalizes = ('LightBeam',)

    state_mapping = {State.ON: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.triggerOnce()

    @synchronize
    def stop(self):
        """Stop acquisition"""
