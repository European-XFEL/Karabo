from functools import partial

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
