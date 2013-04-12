# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Apr 11, 2013 4:20:13 PM$"

from libkarathon import *

class Configurator(object):
    _instance = None
    registry = {}   # { classid_of_base_class : registry_for_classes_derived_of_the_base_class }
    
    @staticmethod
    def registerAsBaseClass(theClass):
        if theClass.__classid__ not in Configurator.registry:
            theClass.__base_classid__ = theClass.__classid__
            Configurator.registry[theClass.__classid__] = {}
        Configurator.registry[theClass.__classid__][theClass.__classid__] = theClass  # self-registering
        
    def __init__(self, baseclassid):
        if baseclassid not in Configurator.registry:
            raise AttributeError,"Argument is a classid of registered base class"
        self.baseRegistry = Configurator.registry[baseclassid]
        assert baseclassid in self.baseRegistry

        
    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(Configurator, cls).__new__(cls, *args, **kwargs)
        return cls._instance
    
    def registerClass(self, derived):
        self.baseRegistry[derived.__classid__] = derived
        return self
        
    def getSchema(self, classid):
        if classid not in self.baseRegistry:
            raise AttributeError,"Class Id '" + classid + "' not found in the base registry"
        Derived = self.baseRegistry[classid]
            
        # generate list of classes in inheritance order from most derived to base
        def inheritanceGenerator(c):
            while c.__classid__ != c.__base_classid__:
                yield c
                c = c.__base__
            else:
                yield c
        
        clist = []
        for c in inheritanceGenerator(Derived):
            clist.insert(0,c) # base class will be the first!
        schema = Schema(classid, AssemblyRules(AccessType(READ | WRITE | INIT)))
        for theClass in clist:
            try:
                theClass.expectedParameters(schema) # fill schema in order from base to derived
            except AttributeError,e:
                pass
        return schema
            
    def create(self, classid, configuration, validation = True):
        if classId not in self.baseRegistry:
            raise AttributeError,"Unknown classid '" + classid + "' in base registry"
        Derived = self.baseRegistry[classId]
        Base = self.baseRegistry[Derived.__base_classid__]
        schema = Base.getSchema(classid)
        if not validation:
            return Derived(configuration)
        validated = Hash()
        validator = Validator()
        try:
            validator.validate(schema, configuration, validated)
        except RuntimeError,e:
            raise RuntimeError,"Validation Exception: " + e
        return Derived(validated)
    
    def createByConf(self, configuration, validation = True):
        classid = configuration.keys()[0]
        return create(classid, configuration[classid], validation)
        
    def createNode(self, nodename, classid, configuration, validation = True):
        if nodename in configuration:
            return create(classid, configuration[nodename], validation)
        raise AttributeError,"Given nodeName \"" + nodename + "\" is not part of input configuration"
    
    def createChoice(self, choice, configuration, validation = True):
        return createByConf(configuration[choice], validation)
       
    def getRegisteredClasses(self):
        return self.baseRegistry.keys()
    
    @staticmethod
    def getRegisteredBaseClasses():
        return Configurator.registry.keys()
