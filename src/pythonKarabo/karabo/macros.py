from .macro_api.alarms import showAlarms, showInterlocks
from .macro_api.device_interface import listMotors, listCameras, listTriggers


def _create_cli_submodule():
    """Create the namespace used by ikarabo."""
    from karabo.common.api import create_module

    # NOTE: This is the macro api part of the ikarabo namespace
    symbols = (
        showAlarms, showInterlocks, listMotors, listCameras, listTriggers
    )
    module = create_module('karabo.macros.cli', *symbols)
    module.__file__ = __file__  # looks nicer when repr(cli) is used
    return module


cli = _create_cli_submodule()
# Hide our implementation!
del _create_cli_submodule
