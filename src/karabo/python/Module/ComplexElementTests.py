#!/usr/bin/python

from libpyexfel import *


class ComplexElementTests(ModulePy):


  def compute(self):
    '''compute method'''
    print "ComplexElementTests. compute method"
    print "print a: ", self.a


  def expectedParameters(cls,expected):
    '''define parameters needed for this module '''

    print "ComplexElementTests. expectedParameters - in python"

    e = INT32_ELEMENT(expected)
    e.key("a")
    e.displayedName("A int")
    e.description("first element of type INT32")
    e.advanced()
    e.assignmentMandatory()
    e.commit()

    e = CHOICE_ELEMENT_CONNECTION(expected)
    e.key("connection")
    e.displayedName("Connection")
    e.description("The connection to the communication layer")
    e.assignmentOptional().defaultValue("Jms")
    e.commit()

    #e = SINGLE_ELEMENT_CONNECTION(expected)
    #e.key("myConnection")
    #e.classId("Jms")
    #e.displayedName("MyConnection")
    #e.description("Connection as SingleElement")
    #e.assignmentOptional().defaultValue("Jms")
    #e.commit()

    e = SINGLE_ELEMENT_PLUGIN_LOADER(expected)
    e.key("myPluginLoader")
    e.displayedName("MyPluginLoader")
    e.description("PluginLoader as SingleElement")
    e.assignmentOptional().defaultValue("Jms")
    e.commit()

    e = LIST_ELEMENT_DEVICE(expected)
    e.key("autoStart")
    e.displayedName("AutoStartDevice(s)")
    e.description("Full configuration of the device(s) ... LIST-element")
    e.assignmentOptional().noDefaultValue()
    e.commit()

    e = NON_EMPTY_LIST_ELEMENT_DEVICE(expected)
    e.key("autoStart2")
    e.displayedName("AutoStartDevice(s)2")
    e.description("Full configuration of the device(s) ... NONemptyLIST-element")
    e.assignmentOptional().noDefaultValue()
    e.commit()

  def configure(self, conf):
    ''' configure the object from config'''
    self.a = conf.getFromPath("a")

  def create(cls):
    ''' create ComplexElementTests object
        Needed for C++
    '''
    return ComplexElementTests()

  expectedParameters = classmethod(expectedParameters)
  create = classmethod(create)
