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
    getConfigurationFromPast, meshTrajectory, plot,
    splitTrajectory)

# For having simplified access to scan documentation via help(scans)
from .usermacro_api import scans


def _create_cli_submodule():
    """Create the namespace used by ikarabo."""
    from karabo.common.api import create_module

    # NOTE: This is the usermacro part of the ikarabo namespace
    symbols = (
        AcquiredFromLog, AcquiredOffline, AScan, AMesh, AMove, APathScan,
        Closable, Coolable, DScan, DMesh, DMove, GenericProxy, Movable,
        plot, Pumpable, scans, Sensible, TScan
    )
    module = create_module('karabo.usermacros.cli', *symbols)
    module.__file__ = __file__  # looks nicer when repr(cli) is used
    return module

cli = _create_cli_submodule()
del _create_cli_submodule
