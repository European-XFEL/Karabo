#!/usr/bin/python

from libpyexfel import *


class Multiplication(ModulePy):

  
  def compute(self):
    '''multiply two numbers'''
    print "Multiplication. compute - in python"
    print "(a * b): ", self.a, " * ", self.b, ' = ', self.a * self.b


  def expectedParameters(cls,expected):
    '''define parameters needed for this module '''

    print "Multiplication. expectedParameters - in python"

    e = INT32_ELEMENT(expected)
    e.key("a")
    e.displayedName("A")
    e.description("first element")
    e.advanced()
    e.assignmentMandatory()
    e.commit()

    e = INT32_ELEMENT(expected)
    e.key("b")
    e.displayedName("B")
    e.description("second element")
    e.assignmentMandatory()
    e.commit()

  def configure(self, conf):
    ''' configure the object from config'''
    self.a = conf.getFromPath("a")
    self.b = conf.getFromPath("b")

  def create(cls):
    ''' create Multiplication object
        Needed for C++
    '''
    return Multiplication()

  expectedParameters = classmethod(expectedParameters)
  create = classmethod(create)

