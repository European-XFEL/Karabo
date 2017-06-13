#############################################################################
# Author: <andrea.parenti@xfel.eu>
# Created on June 13, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import warnings
import functools


def karabo_deprecated(func):
    """
    This is a decorator which can be used to mark functions
    as deprecated. It will result in a warning being emitted
    when the function is used.

    ## Usage examples ##
    @karabo_deprecated
    def my_func():
        pass

    @other_decorators_must_be_upper
    @karabo_deprecated
    def my_func():
        pass
    """
    @functools.wraps(func)
    def new_func(*args, **kwargs):
        warnings.warn_explicit(
            "Call to deprecated function {}.".format(func.__name__),
            category=DeprecationWarning,
            filename=func.__code__.co_filename,
            lineno=func.__code__.co_firstlineno + 1
        )
        return func(*args, **kwargs)
    return new_func
