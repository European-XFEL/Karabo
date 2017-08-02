""""Generalized interface to the image processor"""
from .genericproxy import Sensible
from karabo.middlelayer import State


class ImageProcessorAsSensible(Sensible):
    """"Generalized interface to the image processor device"""
    generalizes = ['ImageProcessor']

    state_mapping = {
        State.NORMAL: State.ACQUIRING
    }

    @property  # Will be later moved to the Generic Proxy class
    def state(self):
        return self.state_mapping.get(self._proxy.state, self._proxy.state)

    def acquire(self):
        pass  # No action is needed to put imageProcessor in acquisition

    def stop(self):
        pass