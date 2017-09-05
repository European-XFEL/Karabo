"""Generalized interface to the ADC"""

from .genericproxy import Sensible
from .middlelayer import State, synchronize


class AdcAsSensible(Sensible):
    """Generalized interface to the  ADC device"""
    generalizes = ('fastAdc', 'adqDigitizer')
    state_mapping = {State.STARTED: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """ start data acquisition """
        yield from self._proxy.start()
