# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import numpy as np

MAX_PRECISION = -24


def float_to_string(f, precision=2):
    """Create pretty floats for legend items"""
    fmt = f"{{:.{precision}f}}"
    return fmt.format(f).rstrip("0").rstrip(".")


def safe_log10(value):
    """Return the max precision if we receive zero when evaluating a value
       with log10. This is to protect the GUI from -inf."""

    if np.isscalar(value):
        return np.log10(value) if value > 0 else MAX_PRECISION

    if not isinstance(value, np.ndarray):
        value = np.array(value)

    # Calculate log10 for valid values, replace with nans otherwise
    index = value > 0
    result = np.log10(value, where=index)
    result[~index] = np.nan

    return result
