"""Generalized interfaces to Beckhoff devices"""
from .genericproxy import Movable


class BeckhoffMotorAsMovable(Movable):
    """Generalized interface to Beckhoff motors"""
    generalizes = ['BeckhoffSimpleMotor']


class BeckhoffMc2AsMovable(Movable):
    """Generalized interface to Beckhoffi MC2 motors"""
    generalizes = ['BeckhoffMC2Motor']
