"""Generalized interfaces to Cameras
"""
from .genericproxy import Sensible


class CamAsSensible(Sensible):
    """Generalized interface to the Cameras """
    generalizes = ('GenicamBaslerCamera', 'BobcatCamera',
                   'PhotonicScienceCamera', 'LimaBaslerCamera',
                   'LimaSimulatedCamera')

    @property
    def value(self):
        return self._proxy.frameRate


class SimulatedCamAsSensible(Sensible):
    """Generalized interface to a simulated camera"""
    generalizes = ('SimulatedCameraPy')

