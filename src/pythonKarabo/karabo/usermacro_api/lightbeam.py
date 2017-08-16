"""Generalized interface to simulated Lightbeam
"""
from karabo.middlelayer import State
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Sensible


class LightBeamAsSensible(Sensible):
    """Generalized interface to the LightBeam"""
    generalizes = ('LightBeam')

    state_mapping = {State.ON: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.triggerOnce()

    @synchronize
    def stop(self):
        """Stop acquisition"""
