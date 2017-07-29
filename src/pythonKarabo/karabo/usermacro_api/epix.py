"""Generalized interface to ePIX
"""
from genericproxy import Sensible


class LpdAsSensible(Sensible):
    """Generalized interface to the ePIX detector"""
    generalizes = ['ePixReceiver']
