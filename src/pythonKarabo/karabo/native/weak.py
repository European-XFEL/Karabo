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
import weakref

__all__ = ['Weak']


class Weak:
    """ This declares a member variable of a class to be weak

    use as follows:

        class Spam(object):
            ham = Weak()

            def __init__(self, ham):
                self.ham = ham # use like a normal variable

    to define a weak variable ham. As a bonus to standard weak
    references, one may set the value to None.
    """
    __slots__ = ("myid", )

    def __init__(self):
        self.myid = str(id(self))

    def __get__(self, instance, owner):
        if instance is None:
            return self
        ret = instance.__dict__.get(self.myid, None)
        return ret() if ret is not None else ret

    def __set__(self, instance, value):
        if value is None:
            instance.__dict__[self.myid] = None
        else:
            instance.__dict__[self.myid] = weakref.ref(value)

    def __delete__(self, instance):
        instance.__dict__[self.myid] = None
