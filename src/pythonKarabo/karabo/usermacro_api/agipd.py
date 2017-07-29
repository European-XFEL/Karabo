"""Generalized interface to AGIPD
"""
from genericproxy import Sensible


class AgipdAsSensible(Sensible):
    """Generalized interface to the AGIPD detector"""
    generalizes = ['AgipdControl']
