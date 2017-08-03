"""Generalized interface to the spectrometers"""
from karabo.middlelayer import State
from .genericproxy import Sensible


class SpectrometerAsSensible(Sensible):
    """Generalized interface to the hr4000 spectrometer device"""
    generalizes = ('HR4000Spectrometer', 'StsSpectrometer')
    state_mapping = {State.ACTIVE: State.STOPPED}
