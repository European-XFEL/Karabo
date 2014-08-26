# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Aug 2, 2012 10:39:43 AM$"

from karabo.karathon import AssemblyRules, AccessType, READ, WRITE, INIT
from karabo.configurator import Configurator

def KARABO_CLASSINFO(classid, version):
    '''
    This decorator should be placed just before class definition. It adds some variables used
    in KARABO configuration infrastructure. It has two parameters: "classId" and "version" similar
    to corresponding C++ macro:
    Example:
            @KARABO_CLASSINFO("Shape","1.0")
            class Shape(object):
                ...
    '''
    def realDecorator(theClass):
        if isinstance(theClass, type):
            theClass.__classid__ = str(classid)
            theClass.__version__ = version
            if hasattr(theClass, "__base_classid__"):
                Configurator(theClass.__base_classid__).registerClass(theClass)
                theClass.__bases_classid__ = []
                for base in theClass.__bases__:
                    for i in base.__bases_classid__:
                        if i not in theClass.__bases_classid__:
                            theClass.__bases_classid__.append(i)
                theClass.__bases_classid__ = tuple(theClass.__bases_classid__)
        return theClass
    return realDecorator

def KARABO_CONFIGURATION_BASE_CLASS(theClass):
    '''
    This decorator should be placed just before "KARABO_CLASSINFO" decorator.  It registers the class as the base configurable class and adds the following classmethods:
    "create", "createNode", "createChoice", "createList", "getSchema" and "getRegisteredClasses". It has no parameters.
    Example:
            @KARABO_CONFIGURATION_BASE_CLASS
            @KARABO_CLASSINFO("Shape","1.0")
            class Shape(object):
                ...
    '''
    if isinstance(theClass, type):
        Configurator.registerAsBaseClass(theClass)
        def create(cls, *args):
            '''
            The factory classmethod to create the instance of class with "classId" using input "configuration".
            Example:
                    instance = Shape.create("EditableCircle", Hash("radius", 12.345))
                    
            The factory classmethod to create instance according input "configuration".  The configuration should have "classId"
            of class to be created as a root element.
            Example:
                    configuration = Hash("EditableCircle.radius", 12.345)
                    instance = Shape.create(configuration)
            '''
            if len(args) == 1:
                return Configurator(cls.__base_classid__).create(args[0])
            elif len(args) == 2:
                return Configurator(cls.__base_classid__).create(args[0], args[1])
            elif len(args) == 3:
                return Configurator(cls.__base_classid__).create(args[0], args[1], args[2])
            else:
                raise TypeError,"Wrong number of arguments and/or their types"
        create = classmethod(create)
        theClass.create = create
        def createNode(cls, nodename, classid, configuration):
            '''
            The helper classmethod to create instance of class specified by "classId" using sub-configuration specified
            by "nodeName" which has to be a part of input "configuration".
            '''
            return Configurator(cls.__base_classid__).createNode(nodename, classid, configuration)
        createNode = classmethod(createNode)
        theClass.createNode = createNode
        def createChoice(cls, choice, configuration):
            '''
            The helper classmethod to create the instance using "choiceName" and input "configuration".
            '''
            return Configurator(cls.__base_classid__).createChoice(choice, configuration)
        createChoice = classmethod(createChoice)
        theClass.createChoice = createChoice
        def createList(cls, listname, configuration):
            '''
            The helper method to create the list of instances using "listName" as a key to the list of configs in input "configuration".
            The configurations will be validated.
            '''
            return Configurator(cls.__base_classid__).createList(listname, configuration)
        createList = classmethod(createList)
        theClass.createList = createList
        def getSchema(cls, classid, rules = AssemblyRules(AccessType(READ|WRITE|INIT))):
            '''
            Use this classmethod to get schema for class with "classid" using assembly "rules"
            Example:
                    schema = Shape.getSchema("Rectangle")
            '''
            return Configurator(cls.__base_classid__).getSchema(classid, rules)
        getSchema = classmethod(getSchema)
        theClass.getSchema = getSchema
        def getRegisteredClasses(cls): return Configurator(cls.__base_classid__).getRegisteredClasses()
        getRegisteredClasses = classmethod(getRegisteredClasses)
        theClass.getRegisteredClasses = getRegisteredClasses
    return theClass
