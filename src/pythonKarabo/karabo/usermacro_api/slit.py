"""Generalized interfaces to the simulated Slit System
"""
from karabo.middlelayer import sleep, State, waitUntil
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
        self._proxy.targetPositionX = next(itpos)
        self._proxy.targetPositionY = next(itpos)
        self._proxy.targetGapX = next(itpos)
        self._proxy.targetGapY = next(itpos)

        if abs(self._proxy.targetPositionX
               - self._proxy.actualPositionX) > self.fepsilon:
            yield from self._proxy.moveX()
            yield from sleep(0.5)
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)

        if abs(self._proxy.targetPositionY
               - self._proxy.actualPositionY) > self.fepsilon:
            yield from self._proxy.moveY()
            yield from sleep(0.5)
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)

        if abs(self._proxy.targetGapX
               - self._proxy.actualGapX) > self.fepsilon:
            yield from self._proxy.moveGapX()
            yield from sleep(0.5)
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)

        if abs(self._proxy.targetGapY
               - self._proxy.actualGapY) > self.fepsilon:
            yield from self._proxy.moveGapY()
            yield from sleep(0.5)
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)
