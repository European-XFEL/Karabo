# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Aug 2, 2012 10:39:43 AM$"

from libkarathon import *
from configurator import *

def KARABO_CLASSINFO(classid, version):
    def realDecorator(theClass):
        if isinstance(theClass, type):
            theClass.__classid__ = classid
            theClass.__version__ = version
            if hasattr(theClass, "__base_classid__"):
                Configurator(theClass.__base_classid__).registerClass(theClass)
        return theClass
    return realDecorator

def KARABO_CONFIGURATION_BASE_CLASS(theClass):
    if isinstance(theClass, type):
        Configurator.registerAsBaseClass(theClass)
        def create(cls, classid, configuration): return Configurator(cls.__base_classid__).create(classid, configuration)
        create = classmethod(create)
        theClass.create = create
        def createByConf(cls, configuration): return Configurator(cls.__base_classid__).createByConf(configuration)
        createByConf = classmethod(createByConf)
        theClass.createByConf = createByConf
        def createNode(cls, nodename, classid, configuration): return Configurator(cls.__base_classid__).createNode(nodename, classid, configuration)
        createNode = classmethod(createNode)
        theClass.createNode = createNode
        def createChoice(cls, choice, configuration): return Configurator(cls.__base_classid__).createChoice(choice, configuration)
        createChoice = classmethod(createChoice)
        theClass.createChoice = createChoice
        def createList(cls, listname, configuration): return Configurator(cls.__base_classid__).createList(listname, configuration)
        createList = classmethod(createList)
        theClass.createList = createList
        def getSchema(cls, classid, rules = AssemblyRules(AccessType(READ|WRITE|INIT))): return Configurator(cls.__base_classid__).getSchema(classid, rules)
        getSchema = classmethod(getSchema)
        theClass.getSchema = getSchema
        def getRegisteredClasses(cls): return Configurator(cls.__base_classid__).getRegisteredClasses()
        getRegisteredClasses = classmethod(getRegisteredClasses)
        theClass.getRegisteredClasses = getRegisteredClasses
    return theClass
