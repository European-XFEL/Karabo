"""Generalized interface to the spectrometers"""
from karabo.middlelayer import State
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Sensible


class SpectrometerAsSensible(Sensible):
    """Generalized interface to the hr4000 spectrometer device"""
    generalizes = ('HR4000Spectrometer', 'StsSpectrometer')
    state_mapping = {State.ACTIVE: State.STOPPED}

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.acquire()

    @synchronize
    def stop(self):
        """Stop acquisition"""
        yield from self._proxy.stop()
