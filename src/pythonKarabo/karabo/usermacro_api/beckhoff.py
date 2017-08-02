"""Generalized interfaces to Beckhoff devices"""

from .genericproxy import Movable
from karabo.middlelayer import State
from karabo.middlelayer_api.eventloop import synchronize


class BeckhoffMotorAsMovable(Movable):

    state_mapping = {
        State.ON: State.STOPPED,  # needed  for current version of MC2 device
    }

    @property  # Will be later moved to the Generic Proxy class
    def state(self):
        return self.state_mapping.get(self._proxy.state, self._proxy.state)

    """base class to be inherited from for common methods"""
    @synchronize
    def moveto(self, pos):
        """Move to *pos*"""
        self._proxy.targetPosition = pos
        yield from self._proxy.move()

    @synchronize
    def stop(self):
        """Stop"""
        yield from self._proxy.stop()

    @synchronize
    def home(self):
        """Home"""
        yield from self._proxy.home()


class BeckhoffSimpleMotorAsMovable(BeckhoffMotorAsMovable):
    """Generalized interface to Beckhoff motors"""
    generalizes = ['BeckhoffSimpleMotor']
    # generalizes = ['Device']  # use this for testing until
                                # BeckhoffSim classid is fixed


class BeckhoffMc2AsMovable(BeckhoffMotorAsMovable):
    """Generalized interface to Beckhoff MC2 motors"""
    generalizes = ['BeckhoffMC2Motor']
