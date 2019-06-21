from functools import partial

import numpy as np
from PyQt4.QtCore import QRectF, QPointF
from PyQt4.QtGui import (
    QAction, QActionGroup, QBrush, QColor, QIcon, QLinearGradient, QMenu,
    QPainter, QPixmap)

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

    return QRectF(*trans_pos, *trans_size)


def levels_almost_equal(image_level, image_range, rtol=0.01):
    return all([np.abs(np.subtract(lvl, rng, dtype=np.float64)) <= rtol * lvl
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


def beam_profile_table_html(x_peak, y_peak):
    if x_peak is None or y_peak is None:
        return
    x_string = [_to_string(value) for value in x_peak]
    y_string = [_to_string(value) for value in y_peak]

    return HTML_TABLE.format(
        x_ampl=x_string[0], x_maxpos=x_string[1], x_fwhm=x_string[2],
        y_ampl=y_string[0], y_maxpos=y_string[1], y_fwhm=y_string[2])


def _to_string(value):
    return "{:.3g}".format(value) if value is not None else "-"


HTML_TABLE = """
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
