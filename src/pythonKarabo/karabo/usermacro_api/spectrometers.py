"""Generalized interface to the spectrometers"""
from genericproxy import Sensible


class hr4000SpectrometerAsSensible(Sensible):
    """Generalized interface to the hr4000 spectrometeri device"""
    generalizes = ['hr4000Spectrometer']



class StsSpectrometerAsSensible(Sensible):
    """Generalized interface to the STS spectrometer device"""
    generalizes = ['stsSpectrometer']
