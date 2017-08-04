"""Generalized interface to EnergyMax device """

from karabo.middlelayer import State
from karabo.middlelayer_api.proxy import synchronize
from .genericproxy import Sensible


class EnergyMaxAsSensible(Sensible):
    """Generalized interface to EnergyMax class """
    generalizes = ('EnergyMax')

    state_mapping = { State.STARTED: State.ACQUIRING }

    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.start()
