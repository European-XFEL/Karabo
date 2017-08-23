"""Generalized interface to the spectrometers"""
from karabo.middlelayer import State
from karabo.middlelayer_api.device_client import synchronize
from .genericproxy import Sensible


class SpectrometerAsSensible(Sensible):
    """Generalized interface to the hr4000 spectrometer device"""
    generalizes = ('HR4000Spectrometer', 'StsSpectrometer')
    state_mapping = {State.ACTIVE: State.STOPPED}

    @synchronize
    def acquire(self):
        """Continuously acquire as from initialization"""

    @synchronize
    def stop(self):
        """Can't stop it"""

    @property
    def value(self):
        """Return the acquired spectrum"""
        return self._proxy.spectrum
