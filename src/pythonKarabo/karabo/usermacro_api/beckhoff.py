"""Generalized interfaces to Beckhoff devices"""
from karabo.middlelayer import State
from .genericproxy import Movable


class BeckhoffMotorAsMovable(Movable):
    """Generalized interface to Beckhoff motors"""
    generalizes = ('BeckhoffSimpleMotor', 'BeckhoffMC2Motor', 'Device')

    state_mapping = {
        State.STOPPED: State.ON,  # needed  for current version of MC2 device
    }
