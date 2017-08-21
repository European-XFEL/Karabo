"""
Public interface to the User Macro API
"""

from .usermacro_api.genericproxy import (
    Closable, Coolable, GenericProxy, Movable, Pumpable, Sensible
)
from .usermacro_api.generalized import (
    AgipdAsSensible, BeckhoffMotorAsMovable,
    CamAsSensible, EnergyMaxAsSensible,
    EpixAsSensible, ImageProcessorAsSensible, LpdAsSensible
)
from .usermacro_api.pipeline import OutputChannel
from .usermacro_api.scans import (
    AScan, AMesh, AMove, APathScan, DScan, DMesh, DMove, meshTrajectory,
    splitTrajectory, TScan
)
from .usermacro_api.spectrometers import SpectrometerAsSensible
from .usermacro_api.usermacro import UserMacro
from .usermacro_api.runconfiguration import RunConfiguration
from .usermacro_api.utils import getConfigurationFromPast