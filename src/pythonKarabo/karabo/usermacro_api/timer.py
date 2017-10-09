from .genericproxy import Movable

class X2TimerAsMovable(Movable):
    """Generalized interface for the X2TimerML"""
    generalizes = ('X2TimerML',)

    @property
    def position(self):
        return self._proxy.actualPosition.magnitude
