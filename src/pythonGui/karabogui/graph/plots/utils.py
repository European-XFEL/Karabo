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
import lttbc
import numpy as np

from karabo.common.scenemodel.api import PlotType


def get_view_range(plot_item) -> tuple[float, float] | None:
    """Retrieve the viewing range (rect) of a plot item along the X-axis.

    :param plot_item: The plot item from which to get the view range.
    :return: A tuple with the left and right X-axis boundaries,
             or None if unavailable.
    """
    view_box = plot_item.getViewBox()
    has_autorange = (
        view_box is not None and
        (view_box.autoRangeEnabled()[0] or view_box.linkedView(0))
    )
    if has_autorange:
        # Return early if auto-range is enabled
        return None
    view_rect = plot_item.viewRect()
    return view_rect.left(), view_rect.right()


def generate_baseline(
        data: np.ndarray,
        offset: float | int = 0.0,
        step: float | int = 1.0
) -> np.ndarray:
    """Generate a baseline array for vector data.

    :param data: The input data array to determine baseline length.
    :param offset: Starting point for the baseline.
    :param step: Interval between baseline values.
    :return: A NumPy array representing the generated baseline.
    """
    # Ensure step is non-zero to avoid infinite arrays
    step = step or 1.0
    stop = offset + data.size * step
    return np.arange(start=offset, stop=stop, step=step, dtype=np.float64)


_TYPICAL_POINTS = 20000
_CLIP_THRESHOLD = _TYPICAL_POINTS // 100
_STD_THRESHOLD = 5

# [(size, data points)]
_DIMENSION_DOWNSAMPLE = [
    (200000, 30000),
    (300000, 40000),
    (400000, 50000),
    (500000, 60000),
]


def _get_sample_threshold(size: int):
    """Calculate the downsample factor based on data size,
    typically every 10th data point."""
    threshold = _TYPICAL_POINTS
    for d_size, d_points in _DIMENSION_DOWNSAMPLE:
        if size > d_size:
            threshold = d_points
        else:
            # No need to look further!
            break

    return threshold


def generate_down_sample(
        y: np.ndarray, x: np.ndarray | None = None,
        threshold: int | None = None,
        rect: tuple[float, float] | None = None,
        deviation: bool = False) -> tuple[np.ndarray, np.ndarray]:
    """
    Generates a downsampled version of the `y` data for efficient plotting.

    :param y: The original dataset for the `y` values to be downsampled.
    :param x: Optional `x` array to use for sampling; defaults to None.
    :param threshold: Max number of data points to retain after downsampling.
    :param rect: Tuple specifying the view range (e.g., (0, 100)).
    :param deviation: If True, considers standard deviation in downsampling.
    :return: A tuple containing the downsampled `x` and `y` arrays.
    """
    if x is None:
        x = np.arange(len(y))
    size = len(x)

    if rect is not None and size > _CLIP_THRESHOLD:
        x_start = x[0]
        size_d = size - 1
        dx = (x[-1] - x_start) / size_d
        x_min = np.clip(int((rect[0] - x_start) / dx), 0, size_d)
        x_max = np.clip(int((rect[1] - x_start) / dx), 0, size_d)
        x, y = x[x_min:x_max], y[x_min:x_max]
        size = len(x)

    if threshold is None:
        threshold = _get_sample_threshold(size)

    if size > threshold:
        if deviation and np.std(y) < _STD_THRESHOLD:
            threshold //= 10  # Drastically reduce threshold if no signal
        x, y = lttbc.downsample(x, y, threshold)

    return x, y


def generate_curve_options(curves: dict, curve_options: dict,
                           plot_type=PlotType.Curve):
    """Populate missing curve_options with default settings

    Note: Modifies the curve_options dictionary in place.
    """
    for proxy, curve in curves.items():
        key = proxy.key
        if key not in curve_options:
            curve_options[key] = {
                "key": key,
                "pen_color": curve.opts["pen"].color().name(),
                "legend_name": curve.name(),
                "plot_type": plot_type,
            }
    return curve_options
