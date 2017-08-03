"""Generalized interface to the ADC"""
from karabo.middlelayer_api.eventloop import synchronize
from karabo.middlelayer import State

from .genericproxy import Sensible


class AdcAsSensible(Sensible):
    """Generalized interface to the  ADC device"""
    generalizes = ('fastAdc', 'adqDigitizer')
    state_mapping = {State.STARTED: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """ start data acquisition """
        yield from self._proxy.start()
