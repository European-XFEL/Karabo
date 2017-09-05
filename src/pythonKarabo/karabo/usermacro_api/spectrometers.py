"""Generalized interface to the spectrometers"""
from .genericproxy import Sensible
from .middlelayer import State, synchronize


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
        return (super().value if super().value
                else self._proxy.spectrum)
