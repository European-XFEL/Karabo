#!/usr/bin/python

from libpyexfel import *

class TestWriteConf1(WriterConfig):

  def write(self, conf):
    print "TestWriterConf1: write"

  def expectedParameters(cls):
    print "In TestWriterConf1: expectedParameters"
    expected = Config()
    element = Config()
    element.clear()
    element.key("c")
    element.displayedName("C")
    element.description("first element")
    element.simpleType(Types.Type.INT32)
    element.assignment(Config.AssignmentType.MANDATORY)
    element.expertLevel(Config.ExpertLevelType.MEDIUM)
    expected.addElement(element)


    element.clear()

    element.key("d")
    element.displayedName("D")
    element.description("second element")
    element.simpleType(Types.Type.STRING)
    element.assignment(Config.AssignmentType.MANDATORY)
    element.expertLevel(Config.ExpertLevelType.MEDIUM)
    expected.addElement(element)

    return expected

  def configure(self, conf):
    print "TestWriterConf1: configure"
    self.c = conf.getFromPathAsInt("c")
    self.d = conf.getFromPathAsString("d")

  def create(cls):
    print "TestWriterConf1: create"
    return TestWriteConf1()

  expectedParameters = classmethod(expectedParameters)
  create = classmethod(create)
