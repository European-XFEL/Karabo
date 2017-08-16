"""Generalized interfaces to Beckhoff devices
"""
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Movable


class SlitSystemAsMovable(Movable):
    """Generalized interface to Slit System"""
    generalizes = ('SlitSystem')

    @property
    def position(self):
        """"Position getter """
        return self._proxy.actualPositionX.magnitude

    @synchronize
    def moveto(self, pos):
        """Move to *pos*"""
        self._proxy.targetPositionX = pos
        yield from self._proxy.moveX()
