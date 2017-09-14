""""Generalized interface to the image processor"""
from .middlelayer import State, synchronize
from .genericproxy import Sensible


class ImageProcessorAsSensible(Sensible):
    """"Generalized interface to the image processor device"""
    generalizes = ('ImageProcessor',)
    # Map the image processor state to UNKNOWN in order to ignore it
    # Needed for neglecting its state when coupled with other sensibles
    state_mapping = {State.NORMAL: State.UNKNOWN}

    @synchronize
    def acquire(self):
        """No action is needed to put imageProcessor in acquisition"""

    @synchronize
    def stop(self):
        """Can't stop acquisition once an imageProcessor in instantiated"""

    @property
    def value(self):
        """Return a dict of statistics"""
        val = super().value
        if not val:
            if self._proxy.doMinMaxMean:
                val = {"FrameRate": self._proxy.frameRate,
                       "MinPxValue": self._proxy.minPxValue,
                       "MaxPxValue": self._proxy.maxPxValue,
                       "MeanPxValue": self._proxy.meanPxValue}
            elif self._proxy.doXYSum:
                val = {"FrameRate": self._proxy.frameRate,
                       "XYSumTime": self._proxy.xYSumTime,
                       "ImgX": self._proxy.imgX,
                       "ImgY": self._proxy.imgY}
            else:
                val = self._proxy.framerate

        return val
