#!/usr/bin/python

# NOTE: variable PYTHONPATH must be set before running this script:
# export PYTHONPATH=/path-to-exfelSuite-installation-dir/lib/debug

from libpyexfel import *

conf = Hash()
conf.setFromPath("ModulePy.python.Multiplication.a", 7)
conf.setFromPath("ModulePy.python.Multiplication.b", 5)
print conf.getFromPathAsHASH("ModulePy")

print '====== Module.expectedParameters() ======= \n', Module.expectedParameters()

print '========= create ================='
m = Module.create(conf)

print '========= compute ================'
m.compute()

print "m.getName(): ", m.getName()

print "m.getClassInfo().getClassId()   ", m.getClassInfo().getClassId()
print "m.getClassInfo().getClassName() ", m.getClassInfo().getClassName()
print "m.getClassInfo().getNamespace() ", m.getClassInfo().getNamespace()
print "m.getClassInfo().getLogCategory() ", m.getClassInfo().getLogCategory()

print "Module.classInfo().getClassId()   ", Module.classInfo().getClassId()
print "Module.classInfo().getClassName() ", Module.classInfo().getClassName()
print "Module.classInfo().getNamespace() ", Module.classInfo().getNamespace()
print "Module.classInfo().getLogCategory() ", Module.classInfo().getLogCategory()

print '========= Test ReaderConfig ======'

inputFile = Hash()
inputFile.setFromPath("TextFile.filename", "configurationFile.xml")
print "Read configuration file: ", inputFile.getFromPathAsSTRING("TextFile.filename")

inpt = ReaderHash.create(inputFile)
print "inpt = ReaderHash.create(inputFile)"
configuration = Hash()
print "configuration = Hash()"
inpt.read(configuration)
print "inpt.read(configuration)"
print "Created configuration object: \n", configuration


