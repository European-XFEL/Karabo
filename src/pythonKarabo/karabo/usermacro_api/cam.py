"""Generalized interfaces to Cameras
"""
from .genericproxy import Sensible
from asyncio import coroutine
from karabo.middlelayer import waitUntil, State


class CamAsSensible(Sensible):
    """Generalized interface to the Cameras """
    generalizes = ['GenicamBaslerCamera', 'PhotonicScienceCamera', 'LimaBaslerCamera', 'LimaSimulatedCamera']

    @coroutine
    def acquire(self):
        """ start data acquisition """
        yield from self._proxy.acquire()

    @coroutine
    def stop(self):
        """ start data acquisition """
        yield from self._proxy.stop()
