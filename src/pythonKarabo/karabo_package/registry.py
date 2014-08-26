
""" This module contains a little helper class which allows
to register subclasses of one class """

class Registry(object):
    @classmethod
    def register(cls, name, dict):
        """ This method is called for each subclass that is defined
        for the class this method is overwritten in. The default implementation
        does nothing, nevertheless don't forget to call the superclasses
        register method, as your parent might already have defined it."""
        pass


class MetaRegistry(type):
    def __init__(self, name, bases, dict):
        super(MetaRegistry, self).__init__(name, bases, dict)
        super(self, self).register(name, dict)


class Registry(Registry):
    """ A class to register subclasses """
    __metaclass__ = MetaRegistry
