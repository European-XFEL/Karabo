"""
Generalized interfaces to devices
"""
from .agipd import AgipdAsSensible
from .beckhoff import BeckhoffMotorAsMovable
from .cam import CamAsSensible, TestImagerAsSensible
from .ccmon import CcmonAsSensible
from .doocs import TangerineAsMovable
from .energymax import EnergyMaxAsSensible
from .epix import EpixAsSensible
from .imageprocessor import ImageProcessorAsSensible
from .lpd import LpdAsSensible
from .lightbeam import LightBeamAsSensible
from .mirror import OffsetMirrorAsMovable
from .slit import SlitSystemAsMovable
from .spectrometers import SpectrometerAsSensible
from .xgm import xgmAsSensible
