from .genericproxy import Movable
from .middlelayer import synchronize


class TangerineAsMovable(Movable):
    """Generalized interface for the doocsTangerine phase shifter"""
    generalizes = ('DoocsTangerine',)

    @property
    def position(self):
        return self._proxy.actualTimeshift.magnitude

    @synchronize
    def moveto(self, pos):
        self._proxy.targetTimeshift = pos
        yield from self._proxy.move()
