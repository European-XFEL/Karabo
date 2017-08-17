"""Generalized interfaces to the simulated Slit System
"""
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Movable


class SlitSystemAsMovable(Movable):
    """Generalized interface to Slit System"""
    generalizes = ('SlitSystem')

    @property
    def position(self):
        """"Position getter """
        return (self._proxy.actualPositionX.magnitude,
                self._proxy.actualPositionY.magnitude,
                self._proxy.actualGapX.magnitude,
                self._proxy.actualGapY.magnitude)

    @synchronize
    def moveto(self, pos):
        """Move to a multidimentional pos"""
        itpos = iter(pos)
        self._proxy.targetPositionX = next(itpos)
        self._proxy.targetPositionY = next(itpos)
        self._proxy.targetGapX = next(itpos)
        self._proxy.targetGapY = next(itpos)

        yield from self._proxy.moveX()
        yield from self._proxy.moveY()
        yield from self._proxy.moveGapX()
        yield from self._proxy.moveGapY()
