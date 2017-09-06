"""Generalized interface to Offset Mirrors
"""
from .middlelayer import State, synchronize, waitUntil
from .genericproxy import Movable


class OffsetMirrorAsMovable(Movable):
    """Generalized interface to Offset Mirrors"""
    generalizes = ('OffsetMirror',)
    state_mapping = {State.STOPPED: State.ON}
    fepsilon = 1e-3

    @property
    def position(self):
        """"Position getter """
        return (self._proxy.positionTx.magnitude,
                self._proxy.positionTy.magnitude,
                self._proxy.positionRx.magnitude,
                self._proxy.positionRy.magnitude,
                self._proxy.positionRz.magnitude)

    @synchronize
    def moveto(self, pos):
        """Move to a multidimentional pos"""
        itpos = iter(pos)
        self._proxy.targetTx = Tx = next(itpos)
        self._proxy.targetTy = Ty = next(itpos)
        self._proxy.targetRx = Rx = next(itpos)
        self._proxy.targetRy = Ry = next(itpos)
        self._proxy.targetRz = Rz = next(itpos)

        if abs(self._proxy.positionTx.magnitude
               - Tx) > self.fepsilon:
            yield from self._proxy.movePositionTx()

        if abs(self._proxy.positionTy.magnitude
               - Ty) > self.fepsilon:
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)
            yield from self._proxy.movePositionTy()

        if abs(self._proxy.positionRx.magnitude
               - Rx) > self.fepsilon:
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)
            yield from self._proxy.movePositionRx()

        if abs(self._proxy.positionRy.magnitude
               - Ry) > self.fepsilon:
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)
            yield from self._proxy.movePositionRy()

        if abs(self._proxy.positionRz.magnitude
               - Rz) > self.fepsilon:
            yield from waitUntil(lambda: self._proxy.state != State.MOVING)
            yield from self._proxy.movePositionRz()
