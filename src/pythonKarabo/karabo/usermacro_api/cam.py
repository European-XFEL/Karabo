"""Generalized interfaces to Cameras
"""
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Sensible


class CamAsSensible(Sensible):
    """Generalized interface to the Cameras """
    generalizes = ('GenicamBaslerCamera', 'PhotonicScienceCamera',
                   'LimaBaslerCamera', 'LimaSimulatedCamera')

    @synchronize
    def acquire(self):
        """ start data acquisition """
        yield from self._proxy.acquire()

    @synchronize
    def stop(self):
        """ stop data acquisition """
        yield from self._proxy.stop()
