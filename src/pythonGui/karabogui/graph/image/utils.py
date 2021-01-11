from functools import partial

import numpy as np
from PyQt5.QtCore import QRectF, QPointF
from PyQt5.QtGui import (
    QBrush, QColor, QIcon, QLinearGradient, QPainter, QPixmap)
from PyQt5.QtWidgets import QActionGroup, QAction, QMenu

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
        # TODO: Improve error handling/message
        raise ValueError("Invalid input.")

    image_array = np.arange(dim)

    scale, translate = np.ma.polyfit(image_array, actual_array, deg=1)

    # since translate is also scaled in Qt, it needs to be compensated
    return scale, translate / scale


def map_rect_to_transform(rect, scaling, translation):
    pos = np.array([rect.x(), rect.y()])
    trans_pos = pos * scaling + translation

    size = np.array([rect.width(), rect.height()])
    trans_size = size * scaling

    return QRectF(*(tuple(trans_pos) + tuple(trans_size)))


def levels_almost_equal(image_level, image_range, rtol=0.05):
    tol = rtol * (image_level[1] - image_level[0])
    return all([
        np.abs(np.subtract(lvl, rng, dtype=np.float64)) <= tol
        for lvl, rng, in zip(image_level, image_range)])


def create_icon_from_colormap(colormap):
    """Creates a gradient icon from a given color map

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


def beam_profile_table_html(x_stats, y_stats):
    if not x_stats or not y_stats:
        return ''

    return PROFILE_STATS_HTML.format(
        x_ampl=_to_string(x_stats.get("amplitude")),
        x_maxpos=_to_string(x_stats.get("max_pos")),
        x_fwhm=_to_string(x_stats.get("fwhm")),
        y_ampl=_to_string(y_stats.get("amplitude")),
        y_maxpos=_to_string(y_stats.get("max_pos")),
        y_fwhm=_to_string(y_stats.get("fwhm")))


def _to_string(value):
    return "{:.3g}".format(value) if value is not None else "-"


PROFILE_STATS_HTML = """
<table style='font-size:8px'>
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


def rescale(array, min_value, max_value, low=0.0, high=100.0):
    """Scale the image with a new range. This is particularly used when
       rescaling to (0, 255)."""

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
    """ Byte scales an array (image).

    Byte scaling means converting the input image to uint8 dtype and scaling
    the range to ``(low, high)`` (default 0-255).
    If the input data already has dtype uint8, no scaling is done.
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
    return (bytedata.clip(low, high) + 0.5).astype(np.uint8)


def correct_image_min(image_min, level_min=None):
    """There could be cases that all image pixels have the same value.
    Since a valid range is needed to produce a valid colormap, we correct the
    image minimum for colormap calculation"""
    if level_min is not None and image_min != level_min:
        # we use the minimum level as the minimum image value.
        return level_min
    else:
        # We use the decrement as the minimum image value.
        return image_min - 1
