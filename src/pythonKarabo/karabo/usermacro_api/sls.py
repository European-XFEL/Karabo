""""Generalized interface to SLS detectors"""
from .genericproxy import Sensible


class SlsDetectorAsSensible(Sensible):
    """Generalized interface to SLS detector devices"""
    generalizes = ['GotthardDetector', 'JungfrauDetector']
