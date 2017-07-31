"""Generalized interface to ePIX
"""
from .genericproxy import Sensible


class EpixAsSensible(Sensible):
    """Generalized interface to the ePIX detector"""
    generalizes = ['ePixReceiver']
