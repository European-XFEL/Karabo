from .genericproxy import Sensible
from .middlelayer import State, synchronize


class LpdAsSensible(Sensible):
    """Generalized interface to the LPD detector"""
    generalizes = ('LPDMainControl',)

    @synchronize
    def prepare(self):
        """Get ready for acquisition"""
        if self._proxy.state == State.UNKNOWN:
            raise RuntimeError("Detector is not ready for acquisition")

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()

    @synchronize
    def stop(self):
        """Stop acquisition"""
        yield from self._proxy.stop()
