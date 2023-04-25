# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
