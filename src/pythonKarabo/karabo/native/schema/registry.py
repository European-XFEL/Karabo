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
""" This module contains a little helper class which allows
to register subclasses of one class """

__all__ = ['MetaRegistry', 'Registry']


class Registry:
    @classmethod
    def register(cls, name, dict):
        """ This method is called for each subclass that is defined
        for the class this method is overwritten in. The default implementation
        does nothing, nevertheless don't forget to call the superclasses
        register method, as your parent might already have defined it."""
        pass


class MetaRegistry(type):
    def __init__(self, name, bases, dict):
        super().__init__(name, bases, dict)
        dict.pop("__classcell__", None)
        super(self, self).register(name, dict)


class Registry(Registry, metaclass=MetaRegistry):
    """ A class to register subclasses """
