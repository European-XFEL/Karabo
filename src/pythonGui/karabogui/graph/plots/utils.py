# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import lttbc
import numpy as np


def get_view_range(plot_item):
    """Get the viewing rect of a plot item for the X-Axis"""
    view_range = None
    view_box = plot_item.getViewBox()
    if view_box is None or not view_box.autoRangeEnabled()[0]:
        view_rect = plot_item.viewRect()
        view_range = (view_rect.left(), view_rect.right())
    return view_range


def generate_baseline(data, offset=0.0, step=1.0):
    """Generate a baseline for vector data

    :param data: The actual data to generate a baseline
    :param start: The offset to start the baseline
    :param step: The bin size (step) of the baseline
    """
    # XXX: No matter what, prevent ourselves against zero
    step = step if step else 1.0
    stop = offset + data.size * step

    return np.arange(start=offset, stop=stop, step=step, dtype=np.float64)


TYPICAL_POINTS = 20000
STD_SIGNAL_THRESHOLD = 5

# [(size, data points)]
DIMENSION_DOWNSAMPLE = [
    (200000, 30000),
    (300000, 40000),
    (400000, 50000),
    (500000, 60000),
]


def _get_sample_threshold(size):
    """Calculate the downsample factor by a given data dimension

    Typically every 10th data point is provided!
    """
    threshold = TYPICAL_POINTS
    for dsize, dpoints in DIMENSION_DOWNSAMPLE:
        if size > dsize:
            threshold = dpoints
        else:
            # No need to look further!
            break

    return threshold


def generate_down_sample(y, x=None, threshold=None, rect=None,
                         deviation=False):
    """This method creates a sampled y to keep a plot_item ``live``

    :param y: The actual y data set from which the downsample is generated
    :param x: Optional x array for sampling.
    :param threshold: The threshold of data points
    :param rect: `Tuple` of the view range, e.g. (0, 100)
    :param deviation: Boolean flag to take into account standard deviation
    """

    if x is None:
        size = len(y)
        x = np.arange(size)
    else:
        size = len(x)

    # Get a new threshold if required based on size!
    initial = threshold is not None
    if threshold is None:
        threshold = _get_sample_threshold(size)

    if size > threshold:
        if rect is not None:
            sizec = (size - 1)
            dx = float(x[-1] - x[0]) / sizec
            x_min = np.clip(int((rect[0] - x[0]) / dx), 0, sizec)
            x_max = np.clip(int((rect[1] - x[0]) / dx), 0, sizec)
            x = x[x_min:x_max]
            y = y[x_min:x_max]
            if not initial:
                # Get a new size and recalculate the threshold!
                size = len(x)
                threshold = _get_sample_threshold(size)

        if deviation and np.std(y) < STD_SIGNAL_THRESHOLD:
            # If there is no signal, we can drastically reduce the number of
            # points we have to plot. This is especially important if we have
            # noisy curves!
            threshold = int(threshold / 10)

        x, y = lttbc.downsample(x, y, threshold)

    return x, y
