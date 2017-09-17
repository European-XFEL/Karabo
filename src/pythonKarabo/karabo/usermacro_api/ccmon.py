"""Generalized interface to Clock and Control Monitor
   This allows to operate it as a Sensible while ignoring its
   states
"""
from .genericproxy import Sensible
from .middlelayer import State, synchronize, setWait


class CcmonAsSensible(Sensible):
    """Generalized interface to the Clock and Control Monitor"""
    generalizes = ('Ccmon',)
    state_mapping = {State.STARTED: State.NORMAL}

    @synchronize
    def acquire(self):
        """Start  pseudo-CCmon acquisition"""
        yield from setWait(self._proxy, fast_data=True)

    @synchronize
    def stop(self):
        """Stop pseudo- CCMon acquisition"""
        yield from setWait(self._proxy, fast_data=False)
