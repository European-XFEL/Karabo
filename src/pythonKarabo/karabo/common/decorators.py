#############################################################################
# Author: <andrea.parenti@xfel.eu>
# Created on June 13, 2017
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
import inspect
import warnings
from functools import wraps
from time import perf_counter


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

    @wraps(func)
    def new_func(*args, **kwargs):
        warnings.warn_explicit(
            f"Call to deprecated function {func.__name__}.",
            category=DeprecationWarning,
            filename=func.__code__.co_filename,
            lineno=func.__code__.co_firstlineno + 1,
        )
        return func(*args, **kwargs)

    return new_func


def timeit(func):
    """A timing decorator"""

    @wraps(func)
    def wrapper(*args, **kwargs):
        t_start = perf_counter()
        ret = func(*args, **kwargs)
        elapsed = perf_counter() - t_start
        print(f"{func.__name__} took {elapsed}")
        return ret

    return wrapper


def validate_args(func):
    """A decorator to be used to validate arguments with their annotations

    Usage examples::

        @validate_args
        def my_func(param1: str, param2: (float, int)):
            pass

    The decorator will raise if the function is called with wrong type input.

    Note: Booleans are also valid for integers!
    """

    @wraps(func)
    def wrapper(*args):
        argspec = inspect.getfullargspec(func).args
        for index, name in enumerate(argspec):
            if not isinstance(args[index], func.__annotations__[name]):
                raise ValueError(
                    "Argument {} got wrong input "
                    "type {}".format(name, args[index])
                )
        return func(*args)

    return wrapper
