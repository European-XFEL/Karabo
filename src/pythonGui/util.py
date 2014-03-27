import weakref

class Weak(object):
    def __get__(self, instance, owner):
        return instance.__dict__[self]()


    def __set__(self, instance, value):
        instance.__dict__[self] = weakref.ref(value)


    def __delete__(self, instance):
        del instance.__dict__[self]


class SignalBlocker(object):
    def __init__(self, object):
        self.object = object


    def __enter__(self):
        self.state = self.object.blockSignals(True)


    def __exit__(self, a, b, c):
        self.object.blockSignals(self.state)
