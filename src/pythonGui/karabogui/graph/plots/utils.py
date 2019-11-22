import numpy as np


def get_view_range(plot_item):
    """Get the viewing rect of a plot item for the X-Axis"""
    rect = None
    view_box = plot_item.getViewBox()
    if view_box is None or not view_box.autoRangeEnabled()[0]:
        rect = plot_item.viewRect()
    return rect


STD_SUBSAMPLE = 100
SUBSAMPLE_THRESHOLD = 20
MEAN_THRESHOLD = 10


def generate_baseline(data, offset=0.0, step=1.0):
    """Generate a baseline for vector data

    :param data: The actual data to generate a baseline
    :param start: The offset to start the baseline
    :param step: The bin size (step) of the baseline
    """

    # XXX: No matter what, prevent ourselves against zero
    step = step if step else 1.0

    stop = offset + data.size * step

    return np.arange(start=offset, stop=stop, step=step, dtype=np.float)


def generate_down_sample(data, rect=None, half_samples=6000, deviation=False,
                         base_line=None):
    """This function creates sampled data to keep a plot_item ``live``

    :param data: The actual data to be sampled
    :param half_samples: The samples after which the data gets down sampled
    :param rect: The rect of the interesting data (``None``)
    :param deviation: Mean sample if the standard deviation is not above 5
    """
    if base_line is None:
        size = len(data)
        base_line = np.arange(size)
    else:
        size = len(base_line)

    # NOTE: Only if we are exceed twice the half samples we down sample!
    if (size // half_samples) > 1:
        # NOTE: We can make the data set smaller if the view if provided!
        if rect is not None and size > 1:
            dx = float(base_line[-1] - base_line[0]) / (size - 1)
            x_min = np.clip(int((rect.left() - base_line[0]) / dx), 0,
                            size - 1)
            x_max = np.clip(int((rect.right() - base_line[0]) / dx), 0,
                            size - 1)
            base_line = base_line[x_min:x_max]
            data = data[x_min:x_max]

        size = len(data)
        d_factor = (size // half_samples)
        if deviation and size and np.std(data) < 5:
            # NOTE: A valid signal is has a signal-to-noise ratio larger 5!
            d_factor = max(STD_SUBSAMPLE, d_factor)
        if d_factor > SUBSAMPLE_THRESHOLD:
            base_line = base_line[::d_factor]
            data = data[::d_factor]
        elif d_factor >= MEAN_THRESHOLD:
            n = size // d_factor
            base_line = base_line[:n * d_factor:d_factor]
            data = data[:n * d_factor].reshape(n, d_factor).mean(axis=1)
        elif d_factor > 1:
            # This is the most accurate downsampling, close to the peak value
            n = size // d_factor
            x1 = np.empty((n, 2))
            x1[:] = base_line[:n * d_factor:d_factor, np.newaxis]
            base_line = x1.reshape(n * 2)
            data1 = np.empty((n, 2))
            data2 = data[:n * d_factor].reshape((n, d_factor))
            data1[:, 0] = data2.max(axis=1)
            data1[:, 1] = data2.min(axis=1)
            data = data1.reshape(n * 2)

    return base_line, data
