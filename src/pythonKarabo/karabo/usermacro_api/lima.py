"""Generalized interface to LIMA cameras"""
from genericproxy import Sensible


class LimaBaslerAsSensible(Sensible):
    """Generalized interface to the Lima Basler camera"""
    generalizes = ['LimaBaslerCamera']


class LimaSimulatedAsSensible(Sensible):
    """Generalized interface to the Simulated Lima camera"""
    generalizes = ['LimaSimulatedCamera']
