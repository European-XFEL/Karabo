"""Generalized interfaces to Cameras
"""
from .genericproxy import Sensible


class CamAsSensible(Sensible):
    """Generalized interface to the Cameras """
    generalizes = ('GenicamBaslerCamera', 'BobcatCamera',
                   'PhotonicScienceCamera', 'LimaBaslerCamera',
                   'LimaSimulatedCamera')

    @property
    def value(self):
        return (super().value if super().value
                else self._proxy.frameRate)


class TestImagerAsSensible(Sensible):
    """Generalized interface to a Test imager"""
    generalizes = ('TestImager')

    @property
    def exposureTime(self):
        """No exposure time"""

    @exposureTime.setter
    def exposureTime(self, value):
        """No exposure time"""

    @property
    def value(self):
        return (super().value if super().value
                else self._proxy.triggerCount)
