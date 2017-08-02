"""Generalized interface to the spectrometers"""
from .genericproxy import Sensible
from karabo.middlelayer import State
from karabo.middlelayer_api.eventloop import synchronize


class SpectrometerAsSensible(Sensible):
    """Generalized interface to the hr4000 spectrometer device"""
    generalizes = ['HR4000Spectrometer', 'StsSpectrometer']

    state_mapping = {State.ACTIVE: State.STOPPED}

    @property  # Will be later moved to the Generic Proxy class
    def state(self):
        return self.state_mapping.get(self._proxy.state, self._proxy.state)

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.acquire()

    @synchronize
    def stop(self):
        """Stop acquisition"""
        yield from self._proxy.stop()