"""Generalized interfaces to GenICam Cameras
"""
from genericproxy import Sensible


class PhotonicScienceAsSensible(Sensible):
    """Generalized interface to the Photonic Science Dual Camera"""
    generalizes = ['PhotonicScienceDualCamera']
