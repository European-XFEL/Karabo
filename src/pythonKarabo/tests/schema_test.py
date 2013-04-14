
import unittest
from libkarathon import *
from configuration_test_classes import *


class  Schema_TestCase(unittest.TestCase):
    def setUp(self):
        try:
            self.schema = Schema("MyTest", AssemblyRules(AccessType(READ | WRITE | INIT)))
            TestStruct1.expectedParameters(self.schema)
        except Exception,e:
            self.fail("setUp exception group 1: " + str(e))

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_buildUp(self):
        try:
            schema = Configurator("Shape").getSchema("Circle")
            self.assertEqual(schema.isAccessInitOnly("shadowEnabled"), True)
            self.assertEqual(schema.isAccessInitOnly("radius"), True)
            self.assertEqual(schema.isLeaf("radius"), True)
        except Exception,e:
            self.fail("test_buildUp exception group 1: " + str(e))
        
        try:
            schema = Schema("test")
            GraphicsRenderer1.expectedParameters(schema)
            self.assertEqual(schema.isAccessInitOnly("shapes.circle.radius"), True)
            self.assertEqual(schema.isLeaf("shapes.circle.radius"), True)
        except Exception,e:
            self.fail("test_buildUp exception group 2: " + str(e))
            
        try:
            instance = GraphicsRenderer.create("GraphicsRenderer",
                        Hash("shapes.Circle.radius", 0.5, "color", "red", "antiAlias", "true"))
        except Exception,e:
            self.fail("test_buildUp exception group 3: " + str(e))

    def test_getRootName(self):
        try:
            self.assertEqual(self.schema.getRootName(), "MyTest")
        except Exception,e:
            self.fail("test_getRootName exception group 1: " + str(e))
            
    def test_getTags(self):
        try:
            self.assertEqual(self.schema.getTags("exampleKey1")[0], "hardware")
            self.assertEqual(self.schema.getTags("exampleKey1")[1], "poll")
            self.assertEqual(self.schema.getTags("exampleKey2")[0], "hardware")
            self.assertEqual(self.schema.getTags("exampleKey2")[1], "poll")
            self.assertEqual(self.schema.getTags("exampleKey3")[0], "hardware")
            self.assertEqual(self.schema.getTags("exampleKey3")[1], "set")
            self.assertEqual(self.schema.getTags("exampleKey4")[0], "software")
            self.assertEqual(self.schema.getTags("exampleKey5")[0], "h/w")
            self.assertEqual(self.schema.getTags("exampleKey5")[1], "d.m.y")
        except Exception,e:
            self.fail("test_getTags exception group 1: " + str(e))
        
    def test_getNodeType(self):
        try:
            nodeType = self.schema.getNodeType("exampleKey1")
            self.assertEqual(nodeType, Schema.LEAF)
            self.assertEqual(self.schema.getNodeType("exampleKey5"), Schema.LEAF)
        except Exception,e:
            self.fail("test_getNodeType exception group 1: " + str(e))
        
    def test_getValueType(self):
        try:
            self.assertEqual(self.schema.getValueType("exampleKey1"), Types.STRING)
            self.assertEqual(self.schema.getValueType("exampleKey2"), Types.INT32)
            self.assertEqual(self.schema.getValueType("exampleKey3"), Types.UINT32)
            self.assertEqual(self.schema.getValueType("exampleKey4"), Types.FLOAT)
            self.assertEqual(self.schema.getValueType("exampleKey5"), Types.INT64)
        except Exception,e:
            self.fail("test_getValueType exception group 1: " + str(e))
     
    def test_getAlias(self):
        try:
            self.assertEqual(self.schema.keyHasAlias("exampleKey1"), False)
            self.assertEqual(self.schema.keyHasAlias("exampleKey2"), True)
            self.assertEqual(self.schema.getAliasFromKey("exampleKey2"), 10)
            self.assertEqual(self.schema.getAliasFromKey("exampleKey3"), 5.5)
            self.assertEqual(self.schema.getAliasFromKey("exampleKey4"), "exampleAlias4")
            self.assertEqual(self.schema.getAliasFromKey("exampleKey5"), "exampleAlias5")
        except Exception,e:
            self.fail("test_getAlias exception group 1: " + str(e))
     
    def test_getAccessMode(self):
        try:
            self.assertEqual(self.schema.getAccessMode("exampleKey1"), AccessMode(WRITE))
            self.assertEqual(self.schema.getAccessMode("exampleKey2"), AccessMode(INIT))
            self.assertEqual(self.schema.getAccessMode("exampleKey3"), AccessMode(WRITE))
            self.assertEqual(self.schema.getAccessMode("exampleKey4"), AccessMode(INIT))
            self.assertEqual(self.schema.getAccessMode("exampleKey5"), AccessMode(READ))
        except Exception,e:
            self.fail("test_getAccessMode exception group 1: " + str(e))
    
    def test_getAssignment(self):
        try:
            self.assertEqual(self.schema.getAssignment("exampleKey1"), Schema.OPTIONAL_PARAM)
            self.assertEqual(self.schema.getAssignment("exampleKey2"), Schema.OPTIONAL_PARAM)
            self.assertEqual(self.schema.getAssignment("exampleKey3"), Schema.MANDATORY_PARAM)
            self.assertEqual(self.schema.getAssignment("exampleKey4"), Schema.INTERNAL_PARAM)
            self.assertEqual(self.schema.getAssignment("exampleKey5"), Schema.OPTIONAL_PARAM)
        except Exception,e:
            self.fail("test_getAssignment exception group 1: " + str(e))
    
    def test_getOptions(self):
        try:
            options = self.schema.getOptions("exampleKey1")
            self.assertEqual(options[0], "Radio")
            self.assertEqual(options[1], "Air Condition")
            self.assertEqual(options[2], "Navigation")
            
            self.assertEqual(self.schema.getOptions("exampleKey2")[0], "5")
            self.assertEqual(self.schema.getOptions("exampleKey2")[1], "25")
            self.assertEqual(self.schema.getOptions("exampleKey2")[2], "10")
            
            self.assertEqual(self.schema.getOptions("exampleKey4")[0], "1.11")
            self.assertEqual(self.schema.getOptions("exampleKey4")[1], "-2.22")
            self.assertEqual(self.schema.getOptions("exampleKey4")[2], "5.55")
        except Exception,e:
            self.fail("test_getOptions exception group 1: " + str(e))
    
    def test_getDefaultValue(self):
        try:
            self.assertEqual(self.schema.getDefaultValue("exampleKey1"), "Navigation")
            self.assertEqual(self.schema.getDefaultValue("exampleKey2"), 10)
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey2", Types.STRING), "10")
            self.assertEqual(self.schema.getDefaultValue("exampleKey5"), 1442244)
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey5", Types.STRING), "1442244")
        except Exception,e:
            self.fail("test_getDefaultValue exception group 1: " + str(e))
    
    def test_getAllowedStates(self):
        try:
            allowedStates = self.schema.getAllowedStates("exampleKey3")
            self.assertEqual(allowedStates[0], "AllOk.Started")
            self.assertEqual(allowedStates[1], "AllOk.Stopped")
            self.assertEqual(self.schema.getAllowedStates("exampleKey3")[2], "AllOk.Run.On")
            self.assertEqual(self.schema.getAllowedStates("exampleKey3")[3], "NewState")
        except Exception,e:
            self.fail("test_getAllowedStates exception group 1: " + str(e))
    
    def test_getUnit(self):
        try:
            self.assertEqual(self.schema.getUnit("exampleKey2"), Units.METER)
            self.assertEqual(self.schema.getUnitName("exampleKey2"), "meter")
            self.assertEqual(self.schema.getUnitSymbol("exampleKey2"), "m")
        except Exception,e:
            self.fail("test_getUnit exception group 1: " + str(e))
    
    def test_getMetricPrefix(self):
        try:
            self.assertEqual(self.schema.getMetricPrefix("exampleKey2"), Units.MILLI)
            self.assertEqual(self.schema.getMetricPrefixName("exampleKey2"), "milli")
            self.assertEqual(self.schema.getMetricPrefixSymbol("exampleKey2"), "m")
        except Exception,e:
            self.fail("test_getMetricPrefix exception group 1: " + str(e))
    
    def test_getMinIncMaxInc(self):
        try:
            self.assertEqual(self.schema.getMinInc("exampleKey2"), 5)
            self.assertEqual(self.schema.getMinIncAs("exampleKey2", Types.STRING), "5")
            self.assertEqual(self.schema.getMaxInc("exampleKey2"), 25)
            self.assertEqual(self.schema.getMaxIncAs("exampleKey2", Types.STRING), "25")
        except Exception,e:
            self.fail("test_getMinIncMaxInc exception group 1: " + str(e))
    
    def test_getMinExcMaxExc(self):
        try:
            self.assertEqual(self.schema.getMinExc("exampleKey3"), 10)
            self.assertEqual(self.schema.getMinExcAs("exampleKey3", Types.STRING), "10")
            self.assertEqual(self.schema.getMaxExc("exampleKey3"), 20)
            self.assertEqual(self.schema.getMaxExcAs("exampleKey3", Types.STRING), "20")
        except Exception,e:
            self.fail("test_getMinExcMaxExc exception group 1: " + str(e))
    
    def test_perKeyFunctionality(self):
        try:
            keys = self.schema.getParameters()
            for key in keys:
                if key == "exampleKey1":
                    self.assertEqual(self.schema.hasAssignment(key), True)
                    self.assertEqual(self.schema.isAssignmentOptional(key), True)
                    self.assertEqual(self.schema.hasDefaultValue(key), True)
                    self.assertEqual(self.schema.hasAccessMode(key), True)
                    self.assertEqual(self.schema.isAccessReconfigurable(key), True)
                    self.assertEqual(self.schema.hasOptions(key), True)
                    self.assertEqual(self.schema.hasTags(key), True)
                    self.assertEqual(self.schema.hasUnit(key), False)
                    self.assertEqual(self.schema.hasMetricPrefix(key), False)
                    
                if key == "exampleKey2":
                    self.assertEqual(self.schema.hasDefaultValue(key), True)
                    self.assertEqual(self.schema.hasAccessMode(key), True)
                    self.assertEqual(self.schema.isAccessInitOnly(key), True)
                    self.assertEqual(self.schema.hasOptions(key), True)
                    self.assertEqual(self.schema.hasTags(key), True)
                    self.assertEqual(self.schema.hasAllowedStates(key), False)
                    self.assertEqual(self.schema.hasUnit(key), True)
                    self.assertEqual(self.schema.hasMetricPrefix(key), True)
                    self.assertEqual(self.schema.hasMinInc(key), True)
                    self.assertEqual(self.schema.hasMaxInc(key), True)
                    
                if key == "exampleKey3":
                    self.assertEqual(self.schema.hasAssignment(key), True)
                    self.assertEqual(self.schema.isAssignmentMandatory(key), True)
                    self.assertEqual(self.schema.hasDefaultValue(key), False)
                    self.assertEqual(self.schema.hasOptions(key), False)
                    self.assertEqual(self.schema.hasAllowedStates(key), True)
                    self.assertEqual(self.schema.hasMinExc(key), True)
                    self.assertEqual(self.schema.hasMaxExc(key), True)
                    
                if key == "exampleKey4":
                    self.assertEqual(self.schema.hasDefaultValue(key), False)
                    self.assertEqual(self.schema.isAssignmentInternal(key), True)
                    self.assertEqual(self.schema.hasAccessMode(key), True)
                    self.assertEqual(self.schema.isAccessInitOnly(key), True)
                    
                if key == "exampleKey5":
                    self.assertEqual(self.schema.hasDefaultValue(key), True)
                    self.assertEqual(self.schema.hasAssignment(key), True)
                    self.assertEqual(self.schema.isAssignmentOptional(key), True)
                    self.assertEqual(self.schema.hasAccessMode(key), True)
                    self.assertEqual(self.schema.isAccessReadOnly(key), True)
                    
        except Exception,e:
            self.fail("test_perKeyFunctionality exception group 1: " + str(e))
    
    def test_helpFunction(self):
        pass


if __name__ == '__main__':
    unittest.main()

