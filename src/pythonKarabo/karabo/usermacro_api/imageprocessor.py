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
        """Can't stop acquisition once an imageProcessor in instantiated"""

    @property
    def value(self):
        """Return a dict of statistics"""
        return ({"FrameRate": self._proxy.frameRate,
                 "MinPxValue": self._proxy.minPxValue,
                 "MaxPxValue": self._proxy.maxPxValue,
                 "MeanPxValue": self._proxy.meanPxValue}
                if self._proxy.doMinMaxMean
                else {"FrameRate": self._proxy.frameRate,
                      "XYSumTime": self._proxy.xYSumTime,
                      "ImgX": self._proxy.imgX,
                      "ImgY": self._proxy.imgY}
                if self._proxy.doXYSum
                else self._proxy.frameRate)
