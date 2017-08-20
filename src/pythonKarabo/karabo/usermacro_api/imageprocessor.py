""""Generalized interface to the image processor"""
from karabo.middlelayer import State
from karabo.middlelayer_api.device_client import synchronize
from .genericproxy import Sensible


class ImageProcessorAsSensible(Sensible):
    """"Generalized interface to the image processor device"""
    generalizes = ('ImageProcessor')

    state_mapping = {State.NORMAL: State.ACQUIRING}

    @synchronize
    def acquire(self):
        """No action is needed to put imageProcessor in acquisition"""

    @synchronize
    def stop(self):
        """Can't stop acquisition once an imageprocessor in instantiated"""
