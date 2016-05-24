import weakref


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

    def __get__(self, instance, owner):
        if instance is None:
            return self
        try:
            ret = instance.__dict__[self]
        except KeyError:
            raise AttributeError
        if ret is None:
            return None
        else:
            return ret()

    def __set__(self, instance, value):
        if value is None:
            instance.__dict__[self] = None
        else:
            instance.__dict__[self] = weakref.ref(value)

    def __delete__(self, instance):
        del instance.__dict__[self]
