from .genericproxy import Sensible
from .middlelayer import State, synchronize


class xgmAsSensible(Sensible):
    """Generalized interface to XgmDoocs class """
    generalizes = ('XgmDoocs',)
    state_mapping = {State.ON: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()

    @property
    def value(self):
        """Return a dict of statistics"""
        return (super().value if super().value else
                {"PulseEnergy": self._proxy.pulseEnergy.pulseEnergy,
                 "Conversion": self._proxy.pulseEnergy.conversion,
                 "Wavelength": self._proxy.pulseEnergy.wavelengthUsed
                 })
