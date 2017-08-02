"""Generalized interfaces to Cameras
"""
from .genericproxy import Sensible
from karabo.middlelayer_api.eventloop import synchronize


class CamAsSensible(Sensible):
    """Generalized interface to the Cameras """
    generalizes = ['GenicamBaslerCamera', 'PhotonicScienceCamera',
                   'LimaBaslerCamera', 'LimaSimulatedCamera']

    @property
    def state(self):
        """Get state"""
        return self._proxy.state

    @synchronize
    def acquire(self):
        """ start data acquisition """
        yield from self._proxy.acquire()

    @synchronize
    def stop(self):
        """ stop data acquisition """
        yield from self._proxy.stop()
