"""Generalized interfaces to GenICam Cameras
"""
from genericproxy import Sensible


class GenicamBaslerAsSensible(Sensible):
    """Generalized interface to the GenICam Basler Camera"""
    generalizes = ['GenicamBaslerCamera']


class PhotonicScienceAsSensible(Sensible):
    """Generalized interface to the Photonic Science Dual Camera"""
    generalizes = ['PhotonicScienceDualCamera']
