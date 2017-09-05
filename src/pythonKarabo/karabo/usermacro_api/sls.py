""""Generalized interface to SLS detectors"""
from .genericproxy import Sensible
from .middlelayer import State, synchronize


class SlsDetectorAsSensible(Sensible):
    """Generalized interface to SLS detector devices"""
    generalizes = ('GotthardDetector', 'JungfrauDetector')
    state_mapping = {State.STARTED: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()
