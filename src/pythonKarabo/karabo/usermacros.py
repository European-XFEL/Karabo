"""
Public interface to the User Macro API
"""
from .usermacro_api.dataobjects import (
    AcquiredData, AcquiredFromLog,
    AcquiredOffline, AcquiredOnline
)
from .usermacro_api.genericproxy import (
    Closable, Coolable, GenericProxy, Movable, Pumpable, Sensible
)
from .usermacro_api.generalized import (
    AgipdAsSensible, BeckhoffMotorAsMovable,
    CamAsSensible, EnergyMaxAsSensible, SpectrometerAsSensible,
    EpixAsSensible, ImageProcessorAsSensible, LpdAsSensible,
    TestImagerAsSensible
)
from .usermacro_api.pipeline import OutputChannel
from .usermacro_api.scans import (
    AScan, AMesh, AMove, APathScan, DScan, DMesh, DMove, TScan
)
from .usermacro_api.usermacro import run_in_event_loop, UserMacro
from .usermacro_api.runconfiguration import RunConfiguration
from .usermacro_api.util import (
    getConfigurationFromPast, meshTrajectory, plotLoggedData,
    splitTrajectory)
