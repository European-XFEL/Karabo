"""Generalized interfaces to the simulated Slit System
"""
from karabo.middlelayer import State, waitUntil
from karabo.middlelayer_api.eventloop import synchronize
from .genericproxy import Movable


class SlitSystemAsMovable(Movable):
    """Generalized interface to Slit System"""
    generalizes = ('SlitSystem')
    fepsilon = 1e-3

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
        self._proxy.targetPositionX = targetX = next(itpos)
        self._proxy.targetPositionY = targetY = next(itpos)
        self._proxy.targetGapX = gapX = next(itpos)
        self._proxy.targetGapY = gapY = next(itpos)

        if abs(self._proxy.targetPositionX.magnitude
               - targetX) > self.fepsilon:
            yield from self._proxy.moveX()

        if abs(self._proxy.targetPositionY.magnitude
               - targetY) > self.fepsilon:
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)
            yield from self._proxy.moveY()

        if abs(self._proxy.targetGapX.magnitude
               - gapX) > self.fepsilon:
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)
            yield from self._proxy.moveGapX()

        if abs(self._proxy.targetGapY.magnitude
               - gapY) > self.fepsilon:
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)
            yield from self._proxy.moveGapY()
