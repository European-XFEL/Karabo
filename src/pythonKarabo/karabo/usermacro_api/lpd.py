"""Generalzed interface to LPD
"""
from genericproxy import Sensible


class LpdAsSensible(Sensible):
    """Generalized interface to the LPD detector"""
    generalizes = ['LpdComposite']
