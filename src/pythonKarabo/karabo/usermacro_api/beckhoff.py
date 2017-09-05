"""Generalized interfaces to Beckhoff devices
"""
from asyncio import wait_for

from .genericproxy import Movable
from .middlelayer import State, synchronize, waitUntil


class BeckhoffMotorAsMovable(Movable):
    """Generalized interface to Beckhoff motors"""
    generalizes = ('BeckhoffSimpleMotor', 'BeckhoffMC2Motor')

    state_mapping = {
        State.STOPPED: State.ON,  # needed  for current version of MC2 device
    }

    @synchronize
    def prepare(self):
        """Get ready for acquisition"""
        if self._proxy.state == State.OFF:
            yield from self._proxy.on()
            yield from wait_for(
                waitUntil(lambda: self._proxy.state == State.ON),
                self.preparation_timeout)
