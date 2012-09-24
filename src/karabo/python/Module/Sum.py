#!/usr/bin/python

from libpyexfel import *


class Sum(ModulePy):

  
  def compute(self):
    print "Sum. compute - in python"
    print "(a + b): ", self.a, " + ", self.b, ' = ', self.a + self.b


  def expectedParameters(cls, expected):
    print "Sum.expectedParameters - in python"

    e = INT32_ELEMENT(expected)
    e.key("a")
    e.displayedName("A")
    e.description("first element of sum")
    e.assignmentOptional().defaultValue(20)
    e.commit()


    e = INT32_ELEMENT(expected)
    e.key("b")
    e.displayedName("B")
    e.description("second element of sum")
    e.advanced()
    e.assignmentMandatory()
    e.commit()

  def configure(self, conf):
    #print 'configure - in python'
    self.a = conf.getFromPath("a")
    self.b = conf.getFromPath("b")

  def create(cls):
    return Sum()

  expectedParameters = classmethod(expectedParameters)
  create = classmethod(create)

