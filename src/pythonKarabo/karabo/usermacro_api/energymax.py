"""Generalized interface to EnergyMax device """

from .genericproxy import Sensible
from .middlelayer import State, synchronize


class EnergyMaxAsSensible(Sensible):
    """Generalized interface to EnergyMax class """
    generalizes = ('EnergyMax',)
    state_mapping = {State.STARTED: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()

    @property
    def value(self):
        """Return a dict of statistics"""
        return (super().value if super().value else
                {"Pulse": self._proxy.measurement.pulse,
                 "Min": self._proxy.measurement.min,
                 "Max": self._proxy.measurement.max,
                 "Mean": self._proxy.measurement.mean,
                 "Stdv": self._proxy.measurement.stdv})
