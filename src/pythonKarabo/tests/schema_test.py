
import unittest
from karabo.karathon import *
from .configuration_example_classes import *


class  Schema_TestCase(unittest.TestCase):
    #def setUp(self):
    #    try:
    #        self.schema = cc.TestStruct1.getSchema("TestStruct1")
    #        #self.schema = Schema("MyTest", AssemblyRules(AccessType(READ | WRITE | INIT)))
    #        #TestStruct1.expectedParameters(self.schema)
    #    except Exception as e:
    #        self.fail("setUp exception group 1: " + str(e))

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_buildUp(self):
        try:
            schema = Shape.getSchema("EditableCircle")
            schema = Configurator("Shape").getSchema("Circle")
            self.assertTrue(schema.isAccessInitOnly("shadowEnabled"))
            self.assertTrue(schema.isAccessInitOnly("radius"))
            self.assertTrue(schema.isLeaf("radius"))
        except Exception as e:
            self.fail("test_buildUp exception group 1: " + str(e))
        
        try:
            schema = Schema("test")
            GraphicsRenderer1.expectedParameters(schema)
            self.assertTrue(schema.isAccessInitOnly("shapes.circle.radius"))
            self.assertTrue(schema.isLeaf("shapes.circle.radius"))
        except Exception as e:
            self.fail("test_buildUp exception group 2: " + str(e))
            
        try:
            instance = GraphicsRenderer.create("GraphicsRenderer",
                        Hash("shapes.Circle.radius", 0.5, "color", "red", "antiAlias", "true"))
        except Exception as e:
            self.fail("test_buildUp exception group 3: " + str(e))

    def test_getRootName(self):
        try:
            schema = Schema("MyTest", AssemblyRules(AccessType(READ | WRITE | INIT)))
            TestStruct1.expectedParameters(schema)
            self.assertEqual(schema.getRootName(), "MyTest")
        except Exception as e:
            self.fail("test_getRootName exception: " + str(e))
            
    def test_getTags(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getTags("exampleKey1")[0], "hardware")
            self.assertEqual(schema.getTags("exampleKey1")[1], "poll")
            self.assertEqual(schema.getTags("exampleKey2")[0], "hardware")
            self.assertEqual(schema.getTags("exampleKey2")[1], "poll")
            self.assertEqual(schema.getTags("exampleKey3")[0], "hardware")
            self.assertEqual(schema.getTags("exampleKey3")[1], "set")
            self.assertEqual(schema.getTags("exampleKey4")[0], "software")
            self.assertEqual(schema.getTags("exampleKey5")[0], "h/w")
            self.assertEqual(schema.getTags("exampleKey5")[1], "d.m.y")
        except Exception as e:
            self.fail("test_getTags exception: " + str(e))
        
    def test_setTags(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.hasTags('x'))
            self.assertEqual(schema.getTags('x'), ['IK', 'BH'])
            schema.setTags('x', 'CY,SE')
            self.assertEqual(schema.getTags('x'), ['CY', 'SE'])
        except Exception as e:
            self.fail("test_setTags exception: " + str(e))
            
    def test_getsetExpertLevel(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getRequiredAccessLevel('x'), AccessLevel.EXPERT)
            self.assertEqual(schema.getRequiredAccessLevel('y'), AccessLevel.USER)
            self.assertEqual(schema.getRequiredAccessLevel('a'), AccessLevel.OBSERVER)
            
            schema.setRequiredAccessLevel('x', AccessLevel.ADMIN)
            schema.setRequiredAccessLevel('y', AccessLevel.OPERATOR)
            self.assertEqual(schema.getRequiredAccessLevel('x'), AccessLevel.ADMIN)
            self.assertEqual(schema.getRequiredAccessLevel('y'), AccessLevel.OPERATOR)
        except Exception as e:
            self.fail("test_setTags exception group 1: " + str(e))
            
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey1'), AccessLevel.USER)
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey2'), AccessLevel.OPERATOR)
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey3'), AccessLevel.EXPERT)
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey4'), AccessLevel.ADMIN)
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey5'), AccessLevel.OBSERVER) #default for readOnly
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey10'), AccessLevel.USER) #default for reconfigurable
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey11'), AccessLevel.OBSERVER) #observerAccess in reconfigurable
        except Exception as e:
            self.fail("test_setTags exception group 2: " + str(e))
            
    def test_getNodeType(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            nodeType = schema.getNodeType("exampleKey1")
            self.assertEqual(nodeType, NodeType.LEAF)
            self.assertEqual(schema.getNodeType("exampleKey5"), NodeType.LEAF)
        except Exception as e:
            self.fail("test_getNodeType exception: " + str(e))
        
    def test_getValueType(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getValueType("exampleKey1"), Types.STRING)
            self.assertEqual(schema.getValueType("exampleKey2"), Types.INT32)
            self.assertEqual(schema.getValueType("exampleKey3"), Types.UINT32)
            self.assertEqual(schema.getValueType("exampleKey4"), Types.DOUBLE)
            self.assertEqual(schema.getValueType("exampleKey5"), Types.INT64)
            self.assertEqual(schema.getValueType("exampleKey7"), Types.VECTOR_INT32)
            self.assertEqual(schema.getValueType("exampleKey8"), Types.VECTOR_DOUBLE)
            self.assertEqual(schema.getValueType("exampleKey9"), Types.VECTOR_STRING)
        except Exception as e:
            self.fail("test_getValueType exception: " + str(e))

    def test_getAliasAsString(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getAliasAsString("exampleKey2"), "10")
            self.assertEqual(schema.getAliasAsString("exampleKey3"), "5.500000000000000")
           
            self.assertEqual(schema.getAliasAsString("exampleKey4"), "exampleAlias4")
            self.assertEqual(schema.getAliasAsString("exampleKey5"), "exampleAlias5")
            self.assertEqual(schema.getAliasAsString("exampleKey6"), "1193046,43724")
            self.assertEqual(schema.getAliasAsString("testPath"), "5")
        except Exception as e:
            self.fail("test_getAliasAsString exception: " + str(e))

    def test_keyHasAlias(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertFalse(schema.keyHasAlias("exampleKey1"))
            self.assertTrue(schema.keyHasAlias("exampleKey2"))
            self.assertTrue(schema.keyHasAlias("exampleKey3"))
            self.assertTrue(schema.keyHasAlias("exampleKey4"))
            self.assertTrue(schema.keyHasAlias("exampleKey5"))
            self.assertTrue(schema.keyHasAlias("exampleKey6"))
            self.assertTrue(schema.keyHasAlias("testPath"))
        except Exception as e:
            self.fail("test_keyHasAlias exception: " + str(e))
        
    def test_aliasHasKey(self):
          try:
              schema = TestStruct1.getSchema("TestStruct1")
              self.assertTrue(schema.aliasHasKey(10))
              self.assertTrue(schema.aliasHasKey(5.5))
              self.assertTrue(schema.aliasHasKey("exampleAlias4"))
              self.assertTrue(schema.aliasHasKey("exampleAlias5"))
              self.assertTrue(schema.aliasHasKey([0x00123456, 0x0000aacc]))
              self.assertFalse(schema.aliasHasKey(7))
              self.assertTrue(schema.aliasHasKey(5))
          except Exception as e:
              self.fail("test_aliasHasKey exception: " + str(e))
   
    def test_getAliasFromKey(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getAliasFromKey("exampleKey2"), 10)
            self.assertEqual(schema.getAliasFromKey("exampleKey3"), 5.5)
            self.assertEqual(schema.getAliasFromKey("exampleKey4"), "exampleAlias4")
            self.assertEqual(schema.getAliasFromKey("exampleKey5"), "exampleAlias5")
            self.assertEqual(schema.getAliasFromKey("exampleKey6"), [0x00123456, 0x0000aacc])
            self.assertEqual(schema.getAliasFromKey("testPath"), 5)
        except Exception as e:
            self.fail("test_getAliasFromKey exception: " + str(e))
       
    def test_setAlias(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getAliasFromKey("x"), 10)
            schema.setAlias('x', 'abc')
            self.assertEqual(schema.getAliasFromKey("x"), 'abc')
            schema.setAlias('x', 99)
            self.assertEqual(schema.getAliasFromKey("x"), 99)
        except Exception as e:
            self.fail("test_setAlias exception: " + str(e))
        
    def test_getKeyFromAlias(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getKeyFromAlias(10), "exampleKey2")
            self.assertEqual(schema.getKeyFromAlias(5.5), "exampleKey3")
            self.assertEqual(schema.getKeyFromAlias("exampleAlias4"), "exampleKey4")
            self.assertEqual(schema.getKeyFromAlias("exampleAlias5"), "exampleKey5")
            self.assertEqual(schema.getKeyFromAlias([0x00123456, 0x0000aacc]), "exampleKey6")
            self.assertEqual(schema.getKeyFromAlias(5), "testPath")
        except Exception as e:
            self.fail("test_KeyFromAlias exception: " + str(e))
   
    def test_getAccessMode(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getAccessMode("exampleKey1"), AccessType.WRITE)
            self.assertEqual(schema.getAccessMode("exampleKey2"), AccessType.INIT)
            self.assertEqual(schema.getAccessMode("exampleKey3"), AccessType.WRITE)
            self.assertEqual(schema.getAccessMode("exampleKey4"), AccessType.INIT)
            self.assertEqual(schema.getAccessMode("exampleKey5"), AccessType.READ)
            self.assertEqual(schema.getAccessMode("exampleKey8"), AccessType.READ)
            self.assertEqual(schema.getAccessMode("exampleKey10"), AccessType.WRITE)
            self.assertEqual(schema.getAccessMode("testPath"), AccessType.WRITE)
            self.assertEqual(schema.getAccessMode("testPath2"), AccessType.READ)
            self.assertEqual(schema.getAccessMode("testPath3"), AccessType.INIT)
        except Exception as e:
            self.fail("test_getAccessMode exception: " + str(e))
    
    def test_getAssignment(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getAssignment("exampleKey1"), AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("exampleKey2"), AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("exampleKey3"), AssignmentType.MANDATORY)
            self.assertEqual(schema.getAssignment("exampleKey4"), AssignmentType.INTERNAL)
            self.assertEqual(schema.getAssignment("exampleKey5"), AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("exampleKey8"), AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("exampleKey10"), AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("testPath"), AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("testPath2"), AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("testPath3"), AssignmentType.MANDATORY)
        except Exception as e:
            self.fail("test_getAssignment exception: " + str(e))
    
    def test_setAssignment(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.hasAssignment('x'))
            self.assertTrue(schema.isAssignmentOptional('x'))
            self.assertFalse(schema.isAssignmentMandatory('x'))
            self.assertEqual(schema.getAssignment('x'), AssignmentType.OPTIONAL)
            schema.setAssignment('x', AssignmentType.MANDATORY)
            self.assertFalse(schema.isAssignmentOptional('x'))
            self.assertTrue(schema.isAssignmentMandatory('x'))
            self.assertEqual(schema.getAssignment('x'), MANDATORY)
        except Exception as e:
            self.fail("test_setAssignment exception: " + str(e))
    
    def test_getOptions(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            options = schema.getOptions("exampleKey1")
            self.assertEqual(options[0], "Radio")
            self.assertEqual(options[1], "Air Condition")
            self.assertEqual(options[2], "Navigation")
            
            self.assertEqual(schema.getOptions("exampleKey2")[0], "5")
            self.assertEqual(schema.getOptions("exampleKey2")[1], "25")
            self.assertEqual(schema.getOptions("exampleKey2")[2], "10")
            
            self.assertEqual(schema.getOptions("exampleKey4")[0], "1.11")
            self.assertEqual(schema.getOptions("exampleKey4")[1], "-2.22")
            self.assertEqual(schema.getOptions("exampleKey4")[2], "5.55")
            
            self.assertEqual(schema.getOptions("testPath")[0], "file1")
            self.assertEqual(schema.getOptions("testPath")[1], "file2")
        except Exception as e:
            self.fail("test_getOptions exception: " + str(e))
    
    def test_setOptions(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            
            options = schema.getOptions("x")
            self.assertEqual(options[0], "5")
            self.assertEqual(options[1], "25")
            self.assertEqual(options[2], "10")
            self.assertEqual(schema.getOptions("x"), ["5", "25", "10"])
            
            schema.setOptions('x', '20, 5, 11, 13, 25')
            options = schema.getOptions("x")
            self.assertEqual(options, ['20', '5', '11', '13', '25'])
        except Exception as e:
            self.fail("test_setOptions exception: " + str(e))
        
    
    def test_getDefaultValue(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getDefaultValue("exampleKey1"), "Navigation")
            self.assertEqual(schema.getDefaultValue("exampleKey2"), 10)
            self.assertEqual(schema.getDefaultValueAs("exampleKey2", Types.STRING), "10")
            self.assertEqual(schema.getDefaultValue("exampleKey5"), 1442244)
            self.assertEqual(schema.getDefaultValueAs("exampleKey5", Types.STRING), "1442244")
            
            self.assertEqual(schema.getDefaultValue("exampleKey6"), 1.11)
            self.assertEqual(schema.getDefaultValueAs("exampleKey6", Types.DOUBLE), 1.11)
            
            self.assertEqual(schema.getDefaultValue("exampleKey7"), [1,2,3])
            self.assertEqual(schema.getDefaultValue("exampleKey8"), [1.1, 2.2, 3.3 ])          
            self.assertEqual(schema.getDefaultValue("exampleKey9"), ["Hallo", "World"])
            
            #'readOnly'-element (vector as well) that does not specify 'initialValue' has 'defaultValue' equal to string "0" :
            self.assertEqual(schema.getDefaultValue("testPath2"), "0")
            self.assertEqual(schema.getDefaultValue("vectInt"), [])
            
            self.assertEqual(schema.getDefaultValue("exampleIntKey"), 20)
            
            self.assertEqual(schema.getDefaultValue("exampleKey5"), 1442244)
            
        except Exception as e:
            self.fail("test_getDefaultValue exception: " + str(e))
    
    def test_setDefaultValue(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.isAssignmentOptional('x'))
            self.assertTrue(schema.hasDefaultValue('x'))
            self.assertEqual(schema.getDefaultValue("x"), 5)
            schema.setDefaultValue("x", 10)
            self.assertEqual(schema.getDefaultValue("x"), 10)
        except Exception as e:
            self.fail("test_setDefaultValue exception: " + str(e))
            
    def test_getAllowedStates(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            allowedStates = schema.getAllowedStates("exampleKey3")
            self.assertEqual(allowedStates[0], "AllOk.Started")
            self.assertEqual(allowedStates[1], "AllOk.Stopped")
            self.assertEqual(schema.getAllowedStates("exampleKey3")[2], "AllOk.Run.On")
            self.assertEqual(schema.getAllowedStates("exampleKey3")[3], "NewState")
            
            self.assertEqual(schema.getAllowedStates("exampleKey7")[0], "Started")
            self.assertEqual(schema.getAllowedStates("exampleKey7")[1], "AllOk")
        except Exception as e:
            self.fail("test_getAllowedStates exception: " + str(e))
    
    def test_getDisplatType(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getDisplayType("exampleBitsKey1"), "bin")
            self.assertEqual(schema.getDisplayType("exampleBitsKey2"), "bin|10:In Error, 21:Busy, 35:HV On, 55:Crate On")
            self.assertEqual(schema.getDisplayType("exampleBitsKey3"), "oct")
            self.assertEqual(schema.getDisplayType("exampleBitsKey4"), "hex")
        except Exception as e:
            self.fail("test_getDisplatType exception: " + str(e))
    
    def test_getUnit(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getUnit("exampleKey2"), Unit.METER)
            self.assertEqual(schema.getUnitName("exampleKey2"), "meter")
            self.assertEqual(schema.getUnitSymbol("exampleKey2"), "m")
        except Exception as e:
            self.fail("test_getUnit exception: " + str(e))
    
    def test_setUnit(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getUnit("x"), Unit.AMPERE)
            schema.setUnit('x', METER)
            self.assertEqual(schema.getUnit("x"), METER)
            self.assertEqual(schema.getUnit("x"), Unit.METER)
            self.assertEqual(schema.getUnitName("x"), "meter")
            self.assertEqual(schema.getUnitSymbol("x"), "m")
        except Exception as e:
            self.fail("test_setUnit exception: " + str(e))
        
    def test_getMetricPrefix(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getMetricPrefix("exampleKey2"), MetricPrefix.MILLI)
            self.assertEqual(schema.getMetricPrefixName("exampleKey2"), "milli")
            self.assertEqual(schema.getMetricPrefixSymbol("exampleKey2"), "m")
        except Exception as e:
            self.fail("test_getMetricPrefix exception: " + str(e))
    
    def test_setMetricPrefix(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getMetricPrefix("x"), MetricPrefix.MILLI)
            schema.setMetricPrefix("x", MetricPrefix.MICRO)
            self.assertEqual(schema.getMetricPrefix("x"), MICRO)
            self.assertEqual(schema.getMetricPrefixName("x"), "micro")
            self.assertEqual(schema.getMetricPrefixSymbol("x"), "u")
        except Exception as e:
            self.fail("test_setMetricPrefix exception: " + str(e))
    
    def test_getMinIncMaxInc(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getMinInc("exampleKey2"), 5)
            self.assertEqual(schema.getMinIncAs("exampleKey2", Types.STRING), "5")
            self.assertEqual(schema.getMaxInc("exampleKey2"), 25)
            self.assertEqual(schema.getMaxIncAs("exampleKey2", Types.STRING), "25")
        except Exception as e:
            self.fail("test_getMinIncMaxInc exception: " + str(e))
    
    def test_setMinIncMaxInc(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getMinInc("x"), 5)
            self.assertEqual(schema.getMinIncAs("x", Types.STRING), "5")
            self.assertEqual(schema.getMaxInc("x"), 25)
            self.assertEqual(schema.getMaxIncAs("x", Types.STRING), "25")
            schema.setMinInc('x', 3)
            schema.setMaxInc('x', 30)
            self.assertEqual(schema.getMinInc("x"), 3)
            self.assertEqual(schema.getMinIncAs("x", Types.STRING), "3")
            self.assertEqual(schema.getMaxInc("x"), 30)
            self.assertEqual(schema.getMaxIncAs("x", Types.STRING), "30")
        except Exception as e:
            self.fail("test_setMinIncMaxInc exception: " + str(e))
    
    def test_getMinExcMaxExc(self):
        schema = TestStruct1.getSchema("TestStruct1")
        try:    
            self.assertEqual(schema.getMinExc("exampleKey3"), 10)
            self.assertEqual(schema.getMinExc("exampleKey4"), -2.22)
        except Exception as e:
            self.fail("test_getMinExcMaxExc exception in getMinExc: " + str(e))
            
        try:    
            self.assertEqual(schema.getMaxExc("exampleKey3"), 20)
            self.assertEqual(schema.getMaxExc("exampleKey4"), 5.55)
        except Exception as e:
            self.fail("test_getMinExcMaxExc exception in getMaxExc: " + str(e))
        
        try:
            self.assertEqual(schema.getMinExcAs("exampleKey3", Types.STRING), "10")
            self.assertEqual(schema.getMinExcAs("exampleKey4", Types.STRING), "-2.220000000000000")
        except Exception as e:
            self.fail("test_getMinExcMaxExc exception in getMinExcAs: " + str(e))
        
        try:    
            self.assertEqual(schema.getMaxExcAs("exampleKey3", Types.STRING), "20")
            self.assertEqual(schema.getMaxExcAs("exampleKey4", Types.STRING), "5.550000000000000")
        except Exception as e:
            self.fail("test_getMinExcMaxExc exception in getMaxExcAs: " + str(e))
    
    def test_setMinExcMaxExc(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getMinExc("y"), 0)
            self.assertEqual(schema.getMaxExc("y"), 29)
            schema.setMinExc("y", 2)
            schema.setMaxExc("y", 30)
            self.assertEqual(schema.getMinExc("y"), 2)
            self.assertEqual(schema.getMaxExc("y"), 30)
        except Exception as e:
            self.fail("test_setMinExcMaxExc exception in getMinExc: " + str(e))

    def test_getWarnAlarmLowHigh(self):
        schema = Configurator(TestStruct1).getSchema("TestStruct1")
        try:    
            self.assertEqual(schema.getWarnLow("exampleKey5"), -10)
            self.assertEqual(schema.getWarnLow("exampleKey6"), -5.5)
            self.assertEqual(schema.getWarnLow("testPath2"), "d")
        except Exception as e:
            self.fail("test_getWarnAlarmLowHigh exception in getWarnLow: " + str(e))
        
        try:
            self.assertEqual(schema.getWarnHigh("exampleKey5"), 10)
            self.assertEqual(schema.getWarnHigh("exampleKey6"), 5.5)
            self.assertEqual(schema.getWarnHigh("testPath2"), "c")
        except Exception as e:
            self.fail("test_getWarnAlarmLowHigh exception in getWarnHigh: " + str(e))
           
        try:
            self.assertEqual(schema.getAlarmLow("exampleKey5"), -20)
            self.assertEqual(schema.getAlarmLow("exampleKey6"), -22.1)
            self.assertEqual(schema.getAlarmLow("testPath2"), "b")
        except Exception as e:
            self.fail("test_getWarnAlarmLowHigh exception in getAlarmLow: " + str(e))
        
        try:
            self.assertEqual(schema.getAlarmHigh("exampleKey5"), 20)
            self.assertEqual(schema.getAlarmHigh("exampleKey6"), 22.777)
            self.assertEqual(schema.getAlarmHigh("testPath2"), "a")
        except Exception as e:
            self.fail("test_getWarnAlarmLowHigh exception in getAlarmHigh: " + str(e))
    
    def test_getWarnAlarmLowHighAs(self):
        schema = Configurator(TestStruct1).getSchema("TestStruct1")
        try:
            self.assertEqual(schema.getWarnLowAs("exampleKey5", Types.STRING), "-10")
            self.assertEqual(schema.getWarnLowAs("exampleKey6", Types.STRING), "-5.500000000000000")
        except Exception as e:
            self.fail("test_getWarnAlarmLowHighAs exception in getWarnLowAs: " + str(e))
        
        try:
            self.assertEqual(schema.getWarnHighAs("exampleKey5", Types.STRING), "10")
            self.assertEqual(schema.getWarnHighAs("exampleKey6", Types.STRING), "5.500000000000000")
        except Exception as e:
            self.fail("test_getWarnAlarmLowHighAs exception in getWarnHighAs: " + str(e))
           
        try:
            self.assertEqual(schema.getAlarmLowAs("exampleKey5", Types.STRING), "-20")
            self.assertEqual(schema.getAlarmLowAs("exampleKey6", Types.STRING), "-22.100000000000001")
        except Exception as e:
            self.fail("test_getWarnAlarmLowHighAs exception in getAlarmLowAs: " + str(e))
        
        try:
            self.assertEqual(schema.getAlarmHighAs("exampleKey5", Types.STRING), "20")
            self.assertEqual(schema.getAlarmHighAs("exampleKey6", Types.STRING), "22.777000000000001")
        except Exception as e:
            self.fail("test_getWarnAlarmLowHighAs exception in getAlarmHighAs: " + str(e))
            
    def test_hasWarnAlarm(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertTrue(schema.hasWarnLow("exampleKey5"))
            self.assertTrue(schema.hasWarnHigh("exampleKey5"))
            
            self.assertTrue(schema.hasWarnLow("exampleKey6"))
            self.assertTrue(schema.hasWarnHigh("exampleKey6"))
            self.assertTrue(schema.hasAlarmLow("exampleKey6"))
            self.assertTrue(schema.hasAlarmHigh("exampleKey6"))
            
            self.assertFalse(schema.hasAlarmHigh("exampleKey1"))
        except Exception as e:
            self.fail("test_hasWarnAlarm exception: " + str(e))         

    def test_vectorElement(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.isAccessReadOnly("exampleKey7"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey7"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey10"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey11"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey12"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey14"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey15"), True)
 
            self.assertEqual(schema.getAlarmLow("exampleKey7"), [-1,-1,-1])
            self.assertEqual(schema.getAlarmHigh("exampleKey7"), [-2,2,-2])
            self.assertEqual(schema.getWarnLow("exampleKey7"), [0,0,0])
            self.assertEqual(schema.getWarnHigh("exampleKey7"), [10,20,30])
            
            self.assertEqual(schema.getAlarmLow("exampleKey8"), [-1.1,-2.2,-3.3])
            self.assertEqual(schema.getWarnHigh("exampleKey8"), [5.5, 7.7, 9.9])
            
            self.assertEqual(schema.getAlarmLow("exampleKey9"), ["a","b"])
            self.assertEqual(schema.getWarnHigh("exampleKey9"), ["c", "d"])
            
            self.assertEqual(schema.getDefaultValue("exampleKey10"), [10, 20, 30])

            self.assertEqual(schema.getDefaultValue("exampleKey12"), [1.1, -2.2, 3.3])
            
            self.assertEqual(schema.getDefaultValue("exampleKey11"), [10, 20, 30])
            self.assertEqual(schema.getDefaultValueAs("exampleKey11", Types.STRING), "10,20,30")
            self.assertEqual(schema.getDefaultValueAs("exampleKey11", Types.VECTOR_INT32), [10, 20, 30])
            
            self.assertEqual(schema.getDefaultValue("exampleKey14"), ["Hallo", "World", "Test"])
            self.assertEqual(schema.getDefaultValueAs("exampleKey14", Types.STRING), "Hallo,World,Test")
 
            self.assertEqual(schema.getDefaultValue("exampleKey15"), ["word1", "word2", "test"])
            self.assertEqual(schema.getDefaultValueAs("exampleKey15", Types.STRING), "word1,word2,test")
 
            self.assertEqual(schema.getMinSize("exampleKey10"), 2)
            self.assertEqual(schema.getMaxSize("exampleKey10"), 7)
        except Exception as e:
            self.fail("test_vectorElement exception: " + str(e))
            
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            validator = Validator()
            configuration = Hash('somelist', [])
            #print "\nInput configuration is ...\n{}".format(configuration)
            validated = validator.validate(schema, configuration)
            #print "Validated configuration is ...\n{}".format(validated)
            somelist = validated['somelist']
            somelist.append(99)
            somelist.append(55)
            configuration['somelist'] = somelist
            validated = validator.validate(schema, configuration)
            #print "After adding to the list the configuration is ...\n{}".format(validated)
        except Exception as e:
            self.fail("test_vectorElement exception 2: " +str(e))
            
    def test_getDisplayType(self):
        try:    
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getDisplayType("testPath"), "fileOut")
            self.assertEqual(schema.getDisplayType("testPath2"), "fileIn")
            self.assertEqual(schema.getDisplayType("testPath3"), "directory")
#            self.assertEqual(schema.getDisplayType("exampleBitsKey1"), "Bitset")
#            self.assertEqual(schema.getDisplayType("exampleBitsKey2"), "Bitset")
#            self.assertEqual(schema.getDisplayType("exampleBitsKey3"), "Bitset")
#            self.assertEqual(schema.getDisplayType("exampleBitsKey4"), "Bitset")
        except Exception as e:
            self.fail("test_getDisplayType exception: " + str(e))  
    
    def test_setDisplayType(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertFalse(schema.hasDisplayType('y'))
            schema.setDisplayType('y', 'blabla')
            self.assertTrue(schema.hasDisplayType('y'))
            self.assertEqual(schema.getDisplayType("y"), "blabla")
        except Exception as e:
            self.fail("test_setDisplayType exception: " + str(e))  
    
    def test_isCommand(self):
        try:  
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertTrue(schema.isCommand("slotTest"))
        except Exception as e:
            self.fail("test_isCommand exception: " + str(e))
            
    def test_isProperty(self):
        try:  
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertFalse(schema.isProperty("slotTest"))
            self.assertTrue(schema.isProperty("testPath2"))
            
        except Exception as e:
            self.fail("test_isProperty exception: " + str(e))

     
    def test_hasArchivePolicy(self):
        try:  
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertTrue(schema.hasArchivePolicy("exampleKey5"))
            self.assertTrue(schema.hasArchivePolicy("exampleKey6"))
            self.assertTrue(schema.hasArchivePolicy("exampleKey7"))
            self.assertTrue(schema.hasArchivePolicy("exampleKey8"))
        except Exception as e:
            self.fail("test_hasArchivePolicy exception: " + str(e))
   
    def test_getArchivePolicy(self):
        try:  
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getArchivePolicy("exampleKey5"), ArchivePolicy.EVERY_EVENT)
            self.assertEqual(schema.getArchivePolicy("exampleKey6"), ArchivePolicy.EVERY_100MS)
            self.assertEqual(schema.getArchivePolicy("exampleKey7"), ArchivePolicy.EVERY_1S)
            self.assertEqual(schema.getArchivePolicy("exampleKey8"), ArchivePolicy.NO_ARCHIVING)
        except Exception as e:
            self.fail("test_getArchivePolicy exception: " + str(e))
   
    def test_setArchivePolicy(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getArchivePolicy("a"), ArchivePolicy.EVERY_100MS)
            schema.setArchivePolicy('a', ArchivePolicy.EVERY_10MIN)
            self.assertEqual(schema.getArchivePolicy("a"), ArchivePolicy.EVERY_10MIN)
        except Exception as e:
            self.fail("test_setArchivePolicy exception: " + str(e))
     
        
    def test_perKeyFunctionality(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            keys = schema.getKeys()
            for key in keys:
                if key == "exampleKey1":
                    self.assertTrue(schema.hasAssignment(key))
                    self.assertTrue(schema.isAssignmentOptional(key))
                    self.assertTrue(schema.hasDefaultValue(key))
                    self.assertTrue(schema.hasAccessMode(key))
                    self.assertTrue(schema.isAccessReconfigurable(key))
                    self.assertTrue(schema.hasOptions(key))
                    self.assertTrue(schema.hasTags(key))
                    self.assertFalse(schema.hasUnit(key))
                    self.assertFalse(schema.hasMetricPrefix(key))
                    
                if key == "exampleKey2":
                    self.assertTrue(schema.hasDefaultValue(key))
                    self.assertTrue(schema.hasAccessMode(key))
                    self.assertTrue(schema.isAccessInitOnly(key))
                    self.assertTrue(schema.hasOptions(key))
                    self.assertTrue(schema.hasTags(key))
                    self.assertFalse(schema.hasAllowedStates(key))
                    self.assertTrue(schema.hasUnit(key))
                    self.assertTrue(schema.hasMetricPrefix(key))
                    self.assertTrue(schema.hasMinInc(key))
                    self.assertTrue(schema.hasMaxInc(key))
                    
                if key == "exampleKey3":
                    self.assertTrue(schema.hasAssignment(key))
                    self.assertTrue(schema.isAssignmentMandatory(key))
                    self.assertFalse(schema.hasDefaultValue(key))
                    self.assertFalse(schema.hasOptions(key))
                    self.assertTrue(schema.hasAllowedStates(key))
                    self.assertTrue(schema.hasMinExc(key))
                    self.assertTrue(schema.hasMaxExc(key))
                    
                if key == "exampleKey4":
                    self.assertFalse(schema.hasDefaultValue(key))
                    self.assertTrue(schema.isAssignmentInternal(key))
                    self.assertTrue(schema.hasAccessMode(key))
                    self.assertTrue(schema.isAccessInitOnly(key))
                    
                if key == "exampleKey5":
                    self.assertTrue(schema.hasDefaultValue(key))
                    self.assertTrue(schema.hasAssignment(key))
                    self.assertTrue(schema.isAssignmentOptional(key))
                    self.assertTrue(schema.hasAccessMode(key))
                    self.assertTrue(schema.isAccessReadOnly(key))   
                    
        except Exception as e:
            self.fail("test_perKeyFunctionality exception group 1: " + str(e))
    
    def test_merge(self):
        try:
            schema = Configurator(Base).getSchema('P1')
            self.assertTrue("a" in schema)
            self.assertFalse("x" in schema)
            self.assertFalse("y" in schema)
            self.assertFalse("z" in schema)
            
            schema2 = Configurator(Base).getSchema('P2')
            self.assertTrue("x" in schema2)
            self.assertTrue("y" in schema2)
            self.assertTrue("z" in schema2)
            
            schema += schema2
            
            self.assertTrue("a" in schema)
            self.assertTrue("x" in schema)
            self.assertTrue("y" in schema)
            self.assertTrue("z" in schema)
            
        except Exception as e:
            self.fail("test_merge exception: " + str(e))
            
        
    def test_logger(self): 
        s1 = Hash("Category.name", "s1", "Category.priority", "DEBUG")
        conf = Hash("categories[0]", s1, "appenders[0].Ostream.layout", "Pattern")
        Logger.configure(conf)
        
        testLog = Logger.getLogger("TestLogA")
        testLog.INFO("This is INFO message")
        testLog.DEBUG("This is DEBUG message") #will not be shown (default priority "INFO")
        testLog.WARN("This is WARN message")
        testLog.ERROR("This is ERROR message")
        
        slog = Logger.getLogger("s1")
        slog.INFO("This is INFO message")
        slog.DEBUG("This is DEBUG message")
        slog.WARN("This is WARN message")
        slog.ERROR("This is ERROR message")
    
    def test_helpFunction(self):
        pass
        #uncomment to see help:
        #schema = TestStruct1.getSchema("TestStruct1")
        #schema.help()

    def test_schemaImageElement(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getDisplayType("myImageElement"), "Image")
            self.assertEqual(schema.getAccessMode("myImageElement"), AccessType.READ)
            self.assertEqual(schema.getNodeType("myImageElement"), NodeType.NODE)
            self.assertEqual(schema.getRequiredAccessLevel("myImageElement"), AccessLevel.OPERATOR) # .operatorAccess()
            self.assertEqual(schema.getDisplayedName("myImageElement"), "myImage")
            self.assertEqual(schema.getDescription("myImageElement"), "Image Element")    
        except Exception as e:
            self.fail("test_schemaImageElement group 1: " + str(e))
            
        try:
            self.assertEqual(schema.getDescription("myImageElement.data"), "Pixel array")
            self.assertEqual(schema.getValueType("myImageElement.data"), Types.VECTOR_CHAR)
            
            self.assertEqual(schema.getDisplayedName("myImageElement.dims"), "Dimensions")
            self.assertEqual(schema.getValueType("myImageElement.dims"), Types.VECTOR_UINT32)
            self.assertEqual(schema.getDisplayType("myImageElement.dims"), "Curve")
            
            self.assertEqual(schema.getDisplayedName("myImageElement.encoding"), "Encoding")
            self.assertEqual(schema.getValueType("myImageElement.encoding"), Types.INT32)
            
            self.assertEqual(schema.getDisplayedName("myImageElement.channelSpace"), "Channel space")
            self.assertEqual(schema.getValueType("myImageElement.channelSpace"), Types.INT32)
            
            self.assertEqual(schema.getDisplayedName("myImageElement.isBigEndian"), "Is big endian")
            self.assertEqual(schema.getValueType("myImageElement.isBigEndian"), Types.BOOL)
            self.assertEqual(schema.getDefaultValue("myImageElement.isBigEndian"), "0")
            
        except Exception as e:
            self.fail("test_schemaImageElement group 2: " + str(e))
            

if __name__ == '__main__':
    unittest.main()

