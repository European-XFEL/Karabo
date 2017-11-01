from functools import partial

from cycler import cycler
from matplotlib.lines import Line2D

from . import const

# register_shortcut will populate this dictionary
_SHORTCUTS = {}  # {(key, classname): (funcname, funcdoc)}


def register_shortcut(func=None, *, key=''):
    """This decorator registers shortcut key, function name, doc string
    combinations
    """
    assert key != '', 'Shortcut key must not be empty!'
    if func is None:
        # In this way the decorator can take kwarg, use partial removes
        # all kwargs and return a function with only one arg
        return partial(register_shortcut, key=key)
    klass, funcname = func.__qualname__.split('.')
    if (key, klass) in _SHORTCUTS:
        # overwrite the shortcut but print a warning
        print("WARNING: overwriting existing shortcut: "
              " {} for {}".format(key, klass))
    doc = func.__doc__ if func.__doc__ else funcname
    _SHORTCUTS[(key, klass)] = (funcname, doc)
    return func


def auto_line2D():
    """Return a dictionary, every new key gets an empty curve with different
    plot style. The iterator is infinite, after exhausts all styles starts
    from the beginning.
    """
    colors = cycler(color=const.COLORS)
    line_styles = cycler(linestyle=const.LINE_STYLES)

    # this order means cycle over color first
    styles = line_styles * colors

    style_iter = styles()  # this an infinite iterator

    def new_line2D():
        while True:
            yield Line2D([], [], picker=const.PICK_RADIUS,
                         lw=const.LINE_WIDTH,
                         # invisible marker but can be picked
                         marker='.', markersize=const.LINE_WIDTH,
                         markeredgecolor='red',
                         markeredgewidth=const.MARKER_EDGE_WIDTH,
                         **(next(style_iter)))

    return new_line2D()
