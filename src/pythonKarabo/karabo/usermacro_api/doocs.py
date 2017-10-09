from .genericproxy import Movable, Sensible
from .middlelayer import State, synchronize


class BeamConditionsAsSensible(Sensible):
    """Generalized interface to DoocsBeamConditions class """
    generalizes = ('DoocsBeamConditions',)
    state_mapping = {State.ON: State.ACQUIRING}

    @property
    def value(self):
        """Return a dict of statistics"""
        return (super().value if super().value else
                {"Energy": self._proxy.energy,
                 "kParameter": self._proxy.kParameter,
                 })


class PhaseshifterAsMovable(Movable):
    """Generalized interface for the phase shifter"""
    generalizes = ('DoocsPhaseshifter',)

    @property
    def position(self):
        return self._proxy.actualTimeshift.magnitude

    @synchronize
    def moveto(self, pos):
        self._proxy.targetTimeshift = pos
        yield from self._proxy.move()


class XgmAsSensible(Sensible):
    """Generalized interface to XgmDoocs class """
    generalizes = ('XgmDoocs',)
    state_mapping = {State.ON: State.ACQUIRING}

    @property
    def value(self):
        """Return a dict of statistics"""
        return (super().value if super().value else
                {"PulseEnergy": self._proxy.pulseEnergy.pulseEnergy,
                 "Conversion": self._proxy.pulseEnergy.conversion,
                 "Wavelength": self._proxy.pulseEnergy.wavelengthUsed
                 })
