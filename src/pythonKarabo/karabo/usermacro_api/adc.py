"""Generalized interface to the ADC"""
from .genericproxy import Sensible
from asyncio import coroutine
from karabo.middlelayer import State


class AdcAsSensible(Sensible):
    """Generalized interface to the fast ADC device"""
    generalizes = ['fastAdc', 'adqDigitizer']
    state_mapping = {State.STARTED: State.ACQUIRING}

    @property  # Will be later moved to the Generic Proxy class
    def state(self):
        return self.state_mapping.get(self._proxy.state, self._proxy.state)

    @coroutine
    def acquire(self):
        """ start data acquisition """
        yield from self._proxy.start()

    @coroutine
    def stop(self):
        """ Stop data acquisition """
        yield from self._proxy.stop()
