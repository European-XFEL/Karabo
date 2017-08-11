"""Generalized interfaces to Cameras
"""
from .genericproxy import Sensible


class CamAsSensible(Sensible):
    """Generalized interface to the Cameras """
    generalizes = ('GenicamBaslerCamera', 'PhotonicScienceCamera',
                   'LimaBaslerCamera', 'LimaSimulatedCamera', 'SimulatedCameraPy')
