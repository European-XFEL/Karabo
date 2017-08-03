""""Generalized interface to SLS detectors"""
from karabo.middlelayer import State
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Sensible


class SlsDetectorAsSensible(Sensible):
    """Generalized interface to SLS detector devices"""
    generalizes = ('GotthardDetector', 'JungfrauDetector')
    state_mapping = {State.STARTED: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()
