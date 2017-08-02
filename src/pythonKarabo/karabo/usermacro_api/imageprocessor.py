""""Generalized interface to the image processor"""
from .genericproxy import Sensible
from karabo.middlelayer import State


class ImageProcessorAsSensible(Sensible):
    """"Generalized interface to the image processor device"""
    generalizes = ['ImageProcessor']

    state_mapping = {
        State.ACQUIRING: State.NORMAL
    }

    def acquire(self):
        pass  # No action is needed to put imageProcessor in acquisition

    def stop(self):
        pass