"""
Public interface to the User Macro API
"""

from .usermacro_api.agipd import AgipdAsSensible
from .usermacro_api.beckhoff import BeckhoffMotorAsMovable
from .usermacro_api.epix import EpixAsSensible
from .usermacro_api.cam import CamAsSensible
from .usermacro_api.lpd import LpdAsSensible
from .usermacro_api.spectrometers import SpectrometerAsSensible
from .usermacro_api.imageprocessor import ImageProcessorAsSensible
from .usermacro_api.energymax import EnergyMaxAsSensible
from .usermacro_api.usermacro import UserMacro
from .usermacro_api.genericproxy import (
    Closable, Coolable, GenericProxy, Movable, Pumpable, Sensible
)
from .usermacro_api.scans import (
    AScan, AMesh, AMove, APathScan, DScan, DMesh, DMove, TScan
)
