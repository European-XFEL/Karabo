# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Apr 11, 2013 4:20:13 PM$"

from karabo.karathon import Hash, Schema, AssemblyRules, AccessType, READ, WRITE, INIT, Validator

class Configurator(object):
    '''
    Configurator is the singleton class that keeps methods for registration and creation other classes that
    support "configuration" protocol, i.e. are configurable classes
    '''
    _instance = None
    registry = {}   # { classid_of_base_class : registry_for_classes_derived_of_the_base_class }
    
    @staticmethod
    def registerAsBaseClass(theClass):
        '''
        if hasattr(theClass, "__classid__"):
            print "registerAsBaseClass: class '{}' has __classid__ = {}".format(theClass.__name__, theClass.__classid__)
        else:
            print "registerAsBaseClass: class '{}' has no __classid__ attribute".format(theClass.__name__)
        if hasattr(theClass, "__base_classid__"):
            print "registerAsBaseClass: class '{}' has __base_classid__ = {}".format(theClass.__name__, theClass.__base_classid__)
        else:
            print "registerAsBaseClass: class '{}' has no __base_classid__ attribute".format(theClass.__name__)
        print "Class '{}.__classid__' found in registry: {}".format(theClass.__name__, theClass.__classid__ in Configurator.registry)
        '''
        if theClass.__classid__ not in Configurator.registry:
            theClass.__base_classid__ = theClass.__classid__
            theClass.__bases_classid__ = (theClass.__classid__,)
            Configurator.registry[theClass.__classid__] = {}
        Configurator.registry[theClass.__classid__][theClass.__classid__] = theClass  # self-registering

    
    def __init__(self, classid):
        '''
        The argument to the constructor may be the classid of a configurable class or configurable class itself:
        Example:
                c = Configurator(ConfigurableClass)
        or
                c = Configurator("ConfigurableClassId")
        '''
        if isinstance(classid, type):
            classid = classid.__classid__
        if not isinstance(classid, basestring):
            raise TypeError("The argument type '{}' is not allowed. "
                            "Must be a class or a str.".format(type(classid)))
        if classid not in Configurator.registry:
            raise AttributeError(
                "Argument is not a class or classid of registered base class")
        self.baseRegistry = Configurator.registry[classid]
        assert classid in self.baseRegistry

        
    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(Configurator, cls).__new__(cls)
        return cls._instance
    
    def registerClass(self, derived):
        self.baseRegistry[derived.__classid__] = derived
        return self
        
    def getSchema(self, classid, rules = AssemblyRules(AccessType(READ | WRITE | INIT))):
        '''
        Get schema for class with "classid" derived from base class given to constructor using assembly "rules"
        Example:
                schema = Configurator(Shape).getSchema("Rectangle")
        '''
        if isinstance(classid, type):
            classid = classid.__classid__
        if not isinstance(classid, str):
            raise TypeError("The first argument type '{}' is not allowed. "
                            "Must be a type or str.".format(type(classid)))
        if classid not in self.baseRegistry:
            raise AttributeError("Class Id '{}' not found in the base registry"
                                 .format(classid))
        Derived = self.baseRegistry[classid]   # userclass -> Derived

        # building list of classes in inheritance order from bases to the last derived
        def inheritanceChain(c, bases_id, clist):
            if not isinstance(c, type):
                return
            if c.__classid__ not in bases_id:
                for x in c.__bases__:
                    inheritanceChain(x, bases_id, clist)
            if c not in clist:
                clist.append(c)
                
        clist = []
        inheritanceChain(Derived, Derived.__bases_classid__, clist)
        # clist contains list of classes in inheritance order
        schema = Schema(classid, rules)
        for theClass in clist:
            try:
                if hasattr(theClass, "expectedParameters"):
                    theClass.expectedParameters(schema) # fill schema in order from base to derived
            except AttributeError as e:
                print "Exception while adding expected parameters for class %r: %r" % (theClass.__name__, e)
        return schema
    
    
    def create(self, *args):
        '''
        The factory method to create the instance of class with "classId" that inherits from base class given to constructor using "input" configuration.
        The last argument is a flag to determine if the input configuration should be validated.
        Example:
                instance = Configurator(Shape).create("EditableCircle", Hash("radius", 12.345))
                
        The factory method to create instance of class that inherits from base class given to constructor using input "configuration".
        The configuration should have "classId" of class to be created as a root element.  The last argument is a flag to determine
        if the input "configuration" should be validated.
        Example:
                configuration = Hash("EditableCircle.radius", 12.345)
                instance = Configurator(Shape).create(configuration)
        '''
        if len(args) == 3:
            classid = args[0]
            configuration = args[1]
            validation = args[2]
        elif len(args) == 2:
            if type(args[1]) == bool:
                configuration = args[0]
                validation = args[1]
                classid = configuration.keys()[0]
                configuration = configuration[classid]
            else:
                classid = args[0]
                configuration = args[1]
                validation = True
        elif len(args) == 1:
            configuration = args[0]
            validation = True
            classid = configuration.keys()[0]
            configuration = configuration[classid]
        else:
            raise TypeError("Wrong number of arguments and/or their types")
        if isinstance(classid, type):
            classid = classid.__classid__
        if not isinstance(classid, basestring):
            raise TypeError(
                "First argument 'classid' must be a python string type")
        if classid not in self.baseRegistry:
            raise AttributeError("Unknown classid '{}' in base registry".
                                 format(classid))
        Derived = self.baseRegistry[classid]
        schema = Configurator(Derived.__base_classid__).getSchema(classid)
        if not validation:
            return Derived(configuration)
        validated = Hash()
        validator = Validator()
        try:
            validated = validator.validate(schema, configuration)
        except RuntimeError as e:
            raise RuntimeError("Validation Exception: " + str(e))
        return Derived(validated)
    
    
    def createNode(self, nodename, classid, configuration, validation = True):
        '''
        The helper method to create instance of class specified by "classId" and derived from class given to constructor using
        sub-configuration specified by "nodeName" which has to be a part of input "configuration".
        The last argument is a flag to determine if the input "configuration" should be validated.
        '''
        if isinstance(classid, type):
            classid = classid.__classid__
        if not isinstance(classid, str):
            raise TypeError(
                "Second argument 'classid' must be a python string type")
        if nodename in configuration:
            return self.create(classid, configuration[nodename], validation)
        raise AttributeError(
            'Given nodeName "{}" is not part of input configuration'.
            format(nodename))

    def createChoice(self, choicename, configuration, validation = True):
        '''
        The helper method to create the instance of class derived from base class given to constructor using "choiceName" and
        input "configuration".  The last argument is a flag to determine if the input configuration should be validated.
        '''
        return self.create(configuration[choicename], validation)
       
    def createList(self, listname, input, validation = True):
        '''
        The helper method to create the list of instances of classes derived from base class given to constructor using "listName"
        used as a key to the list and "input" configuration.  The last argument is a flag to determine if the input configuration
        should be validated.
        '''
        if listname not in input:
            raise AttributeError(
                'Given list name "{}" is not a part of input configuration'.
                format(listname))
        instances = []
        for hash in input[listname]:
            instances.append(create(hash, validation))
        return instances
            
    def getRegisteredClasses(self):
        '''
        Returns list of "classid"'s for all registered classes derived from base class given to constructor.
        '''
        return self.baseRegistry.keys()
    
    @staticmethod
    def getRegisteredBaseClasses():
        '''
        Returns all classid's of base classes registered in Configurator.
        '''
        return Configurator.registry.keys()
