import pint

PINT_INCOMPATIBLE = list(map(int, pint.__version__.split("."))) > [0, 7, 2]
PINT_REASON = "Pint version not compatible with karabo.native"
