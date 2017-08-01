""""Generalized interface to SLS detectors"""
from .genericproxy import Sensible
from karabo.middlelayer import State
from karabo.middlelayer_api.eventloop import synchronize


class SlsDetectorAsSensible(Sensible):
    """Generalized interface to SLS detector devices"""
    generalizes = ['GotthardDetector', 'JungfrauDetector']
    state_mapping = {State.STARTED: State.ACQUIRING}

    @property
    def state(self):
        """Get state"""
        return self.state_mapping.get(self._proxy.state, self._proxy.state)

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()

    @synchronize
    def stop(self):
        """Stop acquisition"""
        yield from self._proxy.stop()
