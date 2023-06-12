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
from functools import partial

import numpy as np
from qtpy.QtCore import QPointF, QRectF
from qtpy.QtGui import (
    QBrush, QColor, QIcon, QLinearGradient, QPainter, QPixmap)
from qtpy.QtWidgets import QAction, QActionGroup, QMenu

from karabogui.graph.common.api import COLORMAPS
from karabogui.graph.common.const import ICON_SIZE

GRADIENT_ICON_SIZE = (50, ICON_SIZE)


def get_transformation(actual_array, dim=0):
    """
    Mapping an image axis to the actual array is not as straightforward.
    PyQtGraph asks for a scale and translate factor (amongst other things).
    Best thing that we can do is get these values from a linear fit:

    y = s(x + t),    where:
        s = scale,
        t = translate,
        x = image array (indices)
        y = actual array

    """

    if len(actual_array) != dim or dim == 0 or actual_array.ndim != 1:
        raise ValueError(f"Cannot calculate transformation with dim {dim} and"
                         f" array dimension {len(actual_array)}.")

    image_array = np.arange(dim)

    scale, translate = np.ma.polyfit(image_array, actual_array, deg=1)

    # Since translate is also scaled in Qt, it needs to be compensated
    return scale, translate / scale


def map_rect_to_transform(rect, scaling, translation):
    pos = np.array([rect.x(), rect.y()])
    trans_pos = pos * scaling + translation

    size = np.array([rect.width(), rect.height()])
    trans_size = size * scaling

    return QRectF(*(tuple(trans_pos) + tuple(trans_size)))


def levels_almost_equal(lut_level, image_level, rtol=0.05):
    """Check if the levels are relatively equal between lut and image levels

    Note: The `lut_level` is typically defined by the colorbar
    """
    tolerance = rtol * (lut_level[1] - lut_level[0])

    equals = [np.abs(np.subtract(lut, im, dtype=np.float64)) <= tolerance
              for lut, im, in zip(lut_level, image_level)]

    return all(equals)


def create_icon_from_colormap(colormap):
    """Return a gradient QIcon from a given color map string

    :param colormap: One of the maps from COLORMAPS
    :type colormap: str

    :returns QIcon
    """
    gradient = QLinearGradient(QPointF(0, 0),
                               QPointF(GRADIENT_ICON_SIZE[0], 0))
    gradient.setStops([(tick, QColor(*color))
                       for tick, color in COLORMAPS[colormap]])
    brush = QBrush(gradient)
    pixmap = QPixmap(*GRADIENT_ICON_SIZE)

    painter = QPainter(pixmap)
    painter.fillRect(QRectF(0, 0, *GRADIENT_ICON_SIZE), brush)
    painter.end()

    return QIcon(pixmap)


def create_colormap_menu(colormaps, current_cmap, on_selection):
    """Create a color map menu for an image widget"""
    menu = QMenu()
    action_group = QActionGroup(menu)

    for colormap in colormaps:
        color_icon = create_icon_from_colormap(colormap)

        action = QAction(menu)
        action.setIconText(colormap)
        action.setIcon(color_icon)
        action.setCheckable(True)
        action.setChecked(colormap == current_cmap)
        action.triggered.connect(partial(on_selection, colormap))

        menu.addAction(action)
        action_group.addAction(action)

    return menu


PROFILE_STATS_HTML = """
<table style='font-size:8pt'>
<tbody>
<tr>
<th align="left">&nbsp;</th><th>x</th><th>y</th>
</tr>
<tr>
<th align="left">Amplitude</th><td>{x_ampl}</td><td>{y_ampl}</td>
</tr>
<tr>
<th align="left">Max Position</th><td>{x_maxpos}</td><td>{y_maxpos}</td>
</tr>
<tr>
<th align="left">FWHM</th><td>{x_fwhm}</td><td>{y_fwhm}</td>
</tr>
</tbody>
</table>
"""


def beam_profile_table_html(x_stats, y_stats):
    """Generate a beam profile table in html"""
    if not x_stats or not y_stats:
        return ''

    def _to_string(value):
        return f"{value:.3g}" if value is not None else "-"

    return PROFILE_STATS_HTML.format(
        x_ampl=_to_string(x_stats.get("amplitude")),
        x_maxpos=_to_string(x_stats.get("max_pos")),
        x_fwhm=_to_string(x_stats.get("fwhm")),
        y_ampl=_to_string(y_stats.get("amplitude")),
        y_maxpos=_to_string(y_stats.get("max_pos")),
        y_fwhm=_to_string(y_stats.get("fwhm")))


def rescale(array, min_value, max_value, low=0.0, high=100.0):
    """Scale the `array` with a new range. This is particularly used when
       rescaling to (0, 255).

    :param array: the array for scaling
    :param min_value: the minimum value of the array
    :param max_value: the maximum value of the array

    :param low: the target scaled minimum
    :param high: the target scaled maximum
    """

    if max_value == min_value:
        return array

    max_value = float(max_value)
    min_value = float(min_value)

    scale = float(high - low) / (max_value - min_value)
    rescaled = scale * array
    rescaled += high - scale * max_value

    rescaled = (array - min_value) * scale + low

    return rescaled


def bytescale(data, cmin=None, cmax=None, low=0, high=255):
    """ Byte scales an array (data).

    Byte scaling means converting the input data to `uint8` dtype and scaling
    the range to ``(low, high)`` (default 0-255).
    """
    high = round(high)
    low = round(low)

    if high > 255:
        raise ValueError(f"high `{high}` should be less than or equal to 255.")
    if low < 0:
        raise ValueError(f"low `{low}` should be greater than or equal to 0.")
    if high < low:
        raise ValueError(f"high `{high}` should be greater than or equal to "
                         f"low `{low}`.")

    if cmin is None:
        cmin = data.min()
    else:
        cmin = float(cmin)

    if cmax is None:
        cmax = data.max()
    else:
        cmax = float(cmax)

    cscale = cmax - cmin
    if cscale < 0:
        raise ValueError(f"cmax {cmax} should be larger than cmin {cmin}.")
    elif cscale == 0:
        cscale = 1

    scale = float(high - low) / cscale
    bytedata = (data - cmin) * scale + low
    bytedata.clip(low, high, out=bytedata)
    bytedata += 0.5
    return bytedata.astype(np.uint8)


def ensure_finite_levels(levels, default_min=0, default_max=255):
    """Make sure the image levels have finite values

    This function assumes the levels are ordered correctly [min, max].

    :returns: tuple of min and max level
    """
    cmin, cmax = levels
    if not np.isfinite(cmin):
        cmin = default_min
    if not np.isfinite(cmax):
        cmax = default_max

    return cmin, cmax


def get_level_limits(image):
    """Retrieve the native level limits for an image value

    :returns: tuple of (min, max) if limits found else `None`
    """
    assert isinstance(image, np.ndarray), "Input value must be an array"

    if np.issubdtype(image.dtype, np.integer):
        info = np.iinfo(image.dtype)
        return info.min, info.max

    elif np.issubdtype(image.dtype, np.floating):
        info = np.finfo(image.dtype)
        return info.min, info.max

    # No limits found!
    return None
