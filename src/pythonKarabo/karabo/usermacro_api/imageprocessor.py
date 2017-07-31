""""Generalized interface to the image processor"""
from .genericproxy import Sensible


class ImageProcessorAsSensible(Sensible):
    """"Generalized interface to the image processor device"""
    generalizes = ['ImageProcessor']
