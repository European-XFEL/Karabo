"""Generalized interfaces to Beckhoff devices"""

from .genericproxy import Movable
from asyncio import coroutine
from genericproxy import Movable
from karabo.middlelayer import waitUntil, State


class BeckhoffMotorAsMovable(Movable):
    """Generalized interface to Beckhoff motors"""
    generalizes = ['BeckhoffSimpleMotor']

    @coroutine
    def moveto(self, pos):
        """Move to *pos*"""
        self._proxy.targetPosition = pos
        yield from waitUntil(lambda: self._proxy.targetPosition == pos)
        yield from self._proxy.move(pos)
        yield from waitUntil(lambda: self._proxy.state != State.MOVING or
                             self._proxy.state.isDerivedFrom(State.NORMAL))
        if self._proxy.state != State.ON:
            # something went wrong
            self._proxy.error("moveto() failed: motor went "
                              "to state " + self._proxy.state.name)

    @coroutine
    def stop(self):
        """Stop"""
        yield from self._proxy.stop()
        yield from waitUntil(lambda: self._proxy.state != State.MOVING or
                             self._proxy.state.isDerivedFrom(State.NORMAL))
        if self._proxy.state != State.ON:
            # something went wrong
            self._proxy.error("moveto() failed: motor went "
                              "to state " + self._proxy.state.name)

    @coroutine
    def step(self, stp):
        """Move of stp steps, up if positive, down if negative"""
        self.current_step = stp
        for i in range(0,abs(stp)):
            if stp > 0:
                yield from self._proxy.stepUp()
            elif stp < 0:
                yield from self._proxy.stepDown()
        if self._proxy.state != State.ON:
            # something went wrong
            self._proxy.error("moveto() failed: motor went "
                              "to state " + self._proxy.state.name)

    @coroutine
    def home(self):
        """Home"""
        yield from self._proxy.home()
        yield from waitUntil(lambda: self._proxy.state != State.MOVING or
                             self._proxy.state.isDerivedFrom(State.NORMAL))
        if self._proxy.state != State.ON:
            # something went wrong
            self._proxy.error("moveto() failed: motor went "
                              "to state " + self._proxy.state.name)


class BeckhoffMc2AsMovable(Movable):
    """Generalized interface to Beckhoffi MC2 motors"""
    generalizes = ['BeckhoffMC2Motor']

