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
# To change this template, choose Tools | Templates
# and open the template in the editor.

from karabind import AssemblyRules, Schema, Validator


class Configurator:
    """Provides factorized configuration

    Configurator is the singleton class that keeps methods for registration and
    creation other classes that are configurable classes
    """
    _instance = None
    registry = {}

    @staticmethod
    def registerAsBaseClass(theClass):
        """Register a class as a base class in the configurator

        :param theClass: to be registered
        """
        if theClass.__classid__ not in Configurator.registry:
            theClass.__base_classid__ = theClass.__classid__
            theClass.__bases_classid__ = (theClass.__classid__,)
            Configurator.registry[theClass.__classid__] = {}
        # self-registering
        shrt = theClass.__classid__
        Configurator.registry[shrt][shrt] = theClass

    def __init__(self, classid):
        """
        The argument to the constructor may be the classid of a configurable
        class or configurable class itself:
        Example:
                c = Configurator(ConfigurableClass)
        or
                c = Configurator("ConfigurableClassId")
        """
        if isinstance(classid, type):
            classid = classid.__classid__
        if not isinstance(classid, str):
            raise TypeError("The argument type '{}' is not allowed. "
                            "Must be a class or a str.".format(type(classid)))
        if classid not in Configurator.registry:
            raise AttributeError(
                "Argument is not a class or classid of registered base class")
        self.baseRegistry = Configurator.registry[classid]
        assert classid in self.baseRegistry

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super().__new__(cls)
        return cls._instance

    def registerClass(self, derived):
        """
        Register a derived class, i.e. a class deriving from a registered
        base class in the configuration system.
        :param derived:
        :return:
        """
        self.baseRegistry[derived.__classid__] = derived
        return self

    def getSchema(self, classid, rules=AssemblyRules()):
        """
        Get schema for class with "classid" derived from base class given to
        constructor using assembly "rules".
        Example:
                schema = Configurator(Shape).getSchema("Rectangle")
        """
        if rules is None:
            rules = AssemblyRules()
        if isinstance(classid, type):
            classid = classid.__classid__
        if not isinstance(classid, str):
            raise TypeError("The first argument type '{}' is not allowed. "
                            "Must be a type or str.".format(type(classid)))
        if classid not in self.baseRegistry:
            raise AttributeError("Class Id '{}' not found in the base registry"
                                 .format(classid))
        Derived = self.baseRegistry[classid]  # userclass -> Derived

        # building list of classes in inheritance order from bases to the
        # last derived
        def inheritanceChain(c, bases_id, clist):
            if not isinstance(c, type):
                return
            classId = getattr(c, '__classid__', None)
            if classId is not None and classId not in bases_id:
                for x in c.__bases__:
                    inheritanceChain(x, bases_id, clist)
            if c not in clist:
                clist.append(c)

        clist = []
        inheritanceChain(Derived, Derived.__bases_classid__, clist)
        # clist contains list of classes in inheritance order
        schema = Schema(classid, rules)
        for theClass in clist:
            if hasattr(theClass, "expectedParameters"):
                # fill schema in order from base to derived
                theClass.expectedParameters(schema)

        return schema

    def create(self, *args):
        """
        The factory method to create the instance of class with "classId" that
        inherits from base class given to constructor using "input"
        configuration.
        The last argument is a flag to determine if the input configuration
        should be validated.

        Example:

                instance = Configurator(Shape).create("EditableCircle",
                                                      Hash("radius", 12.345))

        The factory method to create instance of class that inherits from base
        class given to constructor using input "configuration".
        The configuration should have "classId" of class to be created as a
        root element.  The last argument is a flag to determine if the input
        "configuration" should be validated.

        Example:

                configuration = Hash("EditableCircle.radius", 12.345)
                instance = Configurator(Shape).create(configuration)

        :param args: thus can have an arity of one to three:
                    - class id string
                    - config with classId as root key
                    - class id string, config
                    - config with classId as root key, validation flag
                    - class id string, config, validation flag

        """

        if len(args) == 3:
            classid = args[0]
            configuration = args[1]
            validation = args[2]
        elif len(args) == 2:
            if isinstance(args[1], bool):
                configuration = args[0]
                validation = args[1]
                classid = list(configuration.keys())[0]
                configuration = configuration[classid]
            else:
                classid = args[0]
                configuration = args[1]
                validation = True
        elif len(args) == 1:
            configuration = args[0]
            validation = True
            classid = list(configuration.keys())[0]
            configuration = configuration[classid]
        else:
            raise TypeError("Wrong number of arguments and/or their types")
        if isinstance(classid, type):
            classid = classid.__classid__
        if not isinstance(classid, str):
            raise TypeError("First argument 'classid' must be a python"
                            " string type")
        if classid not in self.baseRegistry:
            raise AttributeError("Unknown classid '{}' in base registry"
                                 .format(classid))
        Derived = self.baseRegistry[classid]

        schema = Configurator(Derived.__base_classid__).getSchema(
            classid, AssemblyRules())
        if not validation:
            return Derived(configuration)
        validator = Validator()
        result, error, validated = validator.validate(schema, configuration)
        if not result:
            raise RuntimeError(f"Validation Exception: {error}")
        return Derived(validated)

    def createNode(self, nodename, classid, configuration, validation=True):
        """
        The helper method to create instance of class specified by "classId"
        and derived from class given to constructor using sub-configuration
        specified by "nodeName" which has to be a part of input
        "configuration".

        The last argument is a flag to determine if the input "configuration"
        should be validated.
        """

        if isinstance(classid, type):
            classid = classid.__classid__
        if not isinstance(classid, str):
            raise TypeError("Second argument 'classid' must be a python"
                            " string type")
        if nodename in configuration:
            return self.create(classid, configuration[nodename], validation)
        raise AttributeError('Given nodeName "{}" is not part of input'
                             ' configuration'.format(nodename))

    def createChoice(self, choicename, configuration, validation=True):
        """
        The helper method to create the instance of class derived from base
        class given to constructor using "choiceName" and
        input "configuration".  The last argument is a flag to determine if
        the input configuration should be validated.
        """

        return self.create(configuration[choicename], validation)

    def createList(self, listname, input, validation=True):
        """
        The helper method to create the list of instances of classes derived
        from base class given to constructor using "listName"
        used as a key to the list and "input" configuration.  The last argument
        is a flag to determine if the input configuration
        should be validated.
        """

        if listname not in input:
            raise AttributeError('Given list name "{}" is not a part of input'
                                 ' configuration'.format(listname))
        instances = []
        for hash in input[listname]:
            instances.append(self.create(hash, validation))
        return instances

    def getRegisteredClasses(self):
        """
        Returns list of "classid"'s for all registered classes derived from
        base class given to constructor.
        """
        return list(self.baseRegistry.keys())

    @staticmethod
    def getRegisteredBaseClasses():
        """
        Returns all classid's of base classes registered in Configurator.
        """
        return list(Configurator.registry.keys())
