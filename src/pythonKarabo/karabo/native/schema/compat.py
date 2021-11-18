import pint

PINT_VERSION = list(map(int, pint.__version__.split(".")))
PINT_INCOMPATIBLE = PINT_VERSION > [0, 7, 2]
PINT_REASON = "Pint version not compatible with karabo.native"
