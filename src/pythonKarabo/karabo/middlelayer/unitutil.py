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
import copy
from asyncio import iscoroutinefunction
from functools import wraps

from karabo.common.states import StateSignifier as SignifierBase
from karabo.native import KaraboValue, newest_timestamp, wrap


class StateSignifier(SignifierBase):
    """Wrapper of the StateSignifier to provide newest timestamp
    """

    def returnMostSignificant(self, iterable):
        ret = super().returnMostSignificant(iterable)
        ret = wrap(copy.copy(ret))
        ret.timestamp = newest_timestamp(iterable)

        return ret


def removeQuantity(func):
    """Decorate a function to remove QuantityValue/KaraboValue input

    This function works as well with async declarations and can be used as::

    @removeQuantity
    def calculate(x, y):
        assert not isinstance(x, KaraboValue)
        assert not isinstance(y, KaraboValue)
        return x, y

    Note: This decorator does not cast to base units! In case of booleans,
          they can be compared by identity.
    """

    def _convert_input(args, kwargs):
        n_args = [
            value.value if isinstance(value, KaraboValue) else value
            for value in tuple(args)]
        n_kwargs = {
            key: value.value if isinstance(value, KaraboValue) else value
            for key, value in kwargs.items()}
        return n_args, n_kwargs

    if iscoroutinefunction(func):

        @wraps(func)
        async def wrapper(*args, **kwargs):
            n_args, n_kwargs = _convert_input(args, kwargs)
            return await func(*n_args, **n_kwargs)
    else:
        @wraps(func)
        def wrapper(*args, **kwargs):
            n_args, n_kwargs = _convert_input(args, kwargs)
            return func(*n_args, **n_kwargs)

    return wrapper
