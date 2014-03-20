import weakref

class Weak(object):
    def __get__(self, instance, owner):
        return instance.__dict__[self]()


    def __set__(self, instance, value):
        instance.__dict__[self] = weakref.ref(value)


    def __delete__(self, instance):
        del instance.__dict__[self]