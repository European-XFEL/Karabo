
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
            self.assertTrue(schema.isAccessInitOnly("shadowEnabled"))
            self.assertTrue(schema.isAccessInitOnly("radius"))
            self.assertTrue(schema.isLeaf("radius"))
        except Exception,e:
            self.fail("test_buildUp exception group 1: " + str(e))
        
        try:
            schema = Schema("test")
            GraphicsRenderer1.expectedParameters(schema)
            self.assertTrue(schema.isAccessInitOnly("shapes.circle.radius"))
            self.assertTrue(schema.isLeaf("shapes.circle.radius"))
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
            self.fail("test_getTags exception: " + str(e))
        
    def test_setTags(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.hasTags('x'))
            self.assertEqual(schema.getTags('x'), ['IK', 'BH'])
            schema.setTags('x', 'CY,SE')
            self.assertEqual(schema.getTags('x'), ['CY', 'SE'])
        except Exception,e:
            self.fail("test_setTags exception: " + str(e))
            
    def test_getsetExpertLevel(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.isExpertLevelAdvanced('x'))
            self.assertFalse(schema.hasExpertLevel('y'))
            schema.setExpertLevel('x', ExpertLevelType.SIMPLE)
            schema.setExpertLevel('y', ExpertLevelType.ADVANCED)
            self.assertTrue(schema.isExpertLevelSimple('x'))
            self.assertTrue(schema.isExpertLevelAdvanced('y'))
        except Exception,e:
            self.fail("test_setTags exception: " + str(e))
            
    def test_getNodeType(self):
        try:
            nodeType = self.schema.getNodeType("exampleKey1")
            self.assertEqual(nodeType, NodeType.LEAF)
            self.assertEqual(self.schema.getNodeType("exampleKey5"), NodeType.LEAF)
        except Exception,e:
            self.fail("test_getNodeType exception: " + str(e))
        
    def test_getValueType(self):
        try:
            self.assertEqual(self.schema.getValueType("exampleKey1"), Types.STRING)
            self.assertEqual(self.schema.getValueType("exampleKey2"), Types.INT32)
            self.assertEqual(self.schema.getValueType("exampleKey3"), Types.UINT32)
            self.assertEqual(self.schema.getValueType("exampleKey4"), Types.DOUBLE)
            self.assertEqual(self.schema.getValueType("exampleKey5"), Types.INT64)
            self.assertEqual(self.schema.getValueType("exampleKey7"), Types.VECTOR_INT32)
            self.assertEqual(self.schema.getValueType("exampleKey8"), Types.VECTOR_DOUBLE)
            self.assertEqual(self.schema.getValueType("exampleKey9"), Types.VECTOR_STRING)
        except Exception,e:
            self.fail("test_getValueType exception: " + str(e))

    def test_getAliasAsString(self):
        try:          
            self.assertEqual(self.schema.getAliasAsString("exampleKey2"), "10")
            self.assertEqual(self.schema.getAliasAsString("exampleKey3"), "5.500000000000000")
           
            self.assertEqual(self.schema.getAliasAsString("exampleKey4"), "exampleAlias4")
            self.assertEqual(self.schema.getAliasAsString("exampleKey5"), "exampleAlias5") 
            self.assertEqual(self.schema.getAliasAsString("testPath"), "5")
        except Exception,e:
            self.fail("test_getAliasAsString exception: " + str(e))

    def test_keyHasAlias(self):
        try:
            self.assertFalse(self.schema.keyHasAlias("exampleKey1"))
            self.assertTrue(self.schema.keyHasAlias("exampleKey2"))
            self.assertTrue(self.schema.keyHasAlias("exampleKey3"))
            self.assertTrue(self.schema.keyHasAlias("exampleKey4"))
            self.assertTrue(self.schema.keyHasAlias("exampleKey5"))
            self.assertTrue(self.schema.keyHasAlias("testPath"))
        except Exception,e:
            self.fail("test_keyHasAlias exception: " + str(e))
        
    def test_aliasHasKey(self):
          try:
              self.assertTrue(self.schema.aliasHasKey(10))
              self.assertTrue(self.schema.aliasHasKey(5.5))
              self.assertTrue(self.schema.aliasHasKey("exampleAlias4"))
              self.assertTrue(self.schema.aliasHasKey("exampleAlias5"))
              self.assertFalse(self.schema.aliasHasKey(7))
              self.assertTrue(self.schema.aliasHasKey(5))
          except Exception,e:
              self.fail("test_aliasHasKey exception: " + str(e))
   
    def test_getAliasFromKey(self):
        try:
            self.assertEqual(self.schema.getAliasFromKey("exampleKey2"), 10)
            self.assertEqual(self.schema.getAliasFromKey("exampleKey3"), 5.5)
            self.assertEqual(self.schema.getAliasFromKey("exampleKey4"), "exampleAlias4")
            self.assertEqual(self.schema.getAliasFromKey("exampleKey5"), "exampleAlias5")
            self.assertEqual(self.schema.getAliasFromKey("testPath"), 5)
        except Exception,e:
            self.fail("test_getAliasFromKey exception: " + str(e))
       
    def test_setAlias(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getAliasFromKey("x"), 10)
            schema.setAlias('x', 'abc')
            self.assertEqual(schema.getAliasFromKey("x"), 'abc')
            schema.setAlias('x', 99)
            self.assertEqual(schema.getAliasFromKey("x"), 99)
        except Exception,e:
            self.fail("test_setAlias exception: " + str(e))
        
    def test_getKeyFromAlias(self):
        try:
            self.assertEqual(self.schema.getKeyFromAlias(10), "exampleKey2")
            self.assertEqual(self.schema.getKeyFromAlias(5.5), "exampleKey3")
            self.assertEqual(self.schema.getKeyFromAlias("exampleAlias4"), "exampleKey4")
            self.assertEqual(self.schema.getKeyFromAlias("exampleAlias5"), "exampleKey5")
            self.assertEqual(self.schema.getKeyFromAlias(5), "testPath")
        except Exception,e:
            self.fail("test_KeyFromAlias exception: " + str(e))
   
    def test_getAccessMode(self):
        try:
            self.assertEqual(self.schema.getAccessMode("exampleKey1"), AccessType.WRITE)
            self.assertEqual(self.schema.getAccessMode("exampleKey2"), AccessType.INIT)
            self.assertEqual(self.schema.getAccessMode("exampleKey3"), AccessType.WRITE)
            self.assertEqual(self.schema.getAccessMode("exampleKey4"), AccessType.INIT)
            self.assertEqual(self.schema.getAccessMode("exampleKey5"), AccessType.READ)
            self.assertEqual(self.schema.getAccessMode("exampleKey8"), AccessType.READ)
            self.assertEqual(self.schema.getAccessMode("exampleKey10"), AccessType.WRITE)
            self.assertEqual(self.schema.getAccessMode("testPath"), AccessType.WRITE)
            self.assertEqual(self.schema.getAccessMode("testPath2"), AccessType.READ)
            self.assertEqual(self.schema.getAccessMode("testPath3"), AccessType.INIT)
        except Exception,e:
            self.fail("test_getAccessMode exception: " + str(e))
    
    def test_getAssignment(self):
        try:
            self.assertEqual(self.schema.getAssignment("exampleKey1"), AssignmentType.OPTIONAL)
            self.assertEqual(self.schema.getAssignment("exampleKey2"), AssignmentType.OPTIONAL)
            self.assertEqual(self.schema.getAssignment("exampleKey3"), AssignmentType.MANDATORY)
            self.assertEqual(self.schema.getAssignment("exampleKey4"), AssignmentType.INTERNAL)
            self.assertEqual(self.schema.getAssignment("exampleKey5"), AssignmentType.OPTIONAL)
            self.assertEqual(self.schema.getAssignment("exampleKey8"), AssignmentType.OPTIONAL)
            self.assertEqual(self.schema.getAssignment("exampleKey10"), AssignmentType.OPTIONAL)
            self.assertEqual(self.schema.getAssignment("testPath"), AssignmentType.OPTIONAL)
            self.assertEqual(self.schema.getAssignment("testPath2"), AssignmentType.OPTIONAL)
            self.assertEqual(self.schema.getAssignment("testPath3"), AssignmentType.MANDATORY)
        except Exception,e:
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
        except Exception,e:
            self.fail("test_setAssignment exception: " + str(e))
    
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
            
            self.assertEqual(self.schema.getOptions("testPath")[0], "file1")
            self.assertEqual(self.schema.getOptions("testPath")[1], "file2")
        except Exception,e:
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
        except Exception,e:
            self.fail("test_setOptions exception: " + str(e))
        
    
    def test_getDefaultValue(self):
        try:
            self.assertEqual(self.schema.getDefaultValue("exampleKey1"), "Navigation")
            self.assertEqual(self.schema.getDefaultValue("exampleKey2"), 10)
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey2", Types.STRING), "10")
            self.assertEqual(self.schema.getDefaultValue("exampleKey5"), 1442244)
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey5", Types.STRING), "1442244")
            
            self.assertEqual(self.schema.getDefaultValue("exampleKey6"), 1.11)
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey6", Types.DOUBLE), 1.11)
            
            self.assertEqual(self.schema.getDefaultValue("exampleKey7"), [1,2,3])
            self.assertEqual(self.schema.getDefaultValue("exampleKey8"), [1.1, 2.2, 3.3 ])          
            self.assertEqual(self.schema.getDefaultValue("exampleKey9"), ["Hallo", "World"])
            
            #'readOnly'-element (vector as well) that does not specify 'initialValue' has 'defaultValue' equal to string "0" :
            self.assertEqual(self.schema.getDefaultValue("testPath2"), "0")
            self.assertEqual(self.schema.getDefaultValue("vectInt"), "0")
            
            self.assertEqual(self.schema.getDefaultValue("exampleIntKey"), 20)
            
            self.assertEqual(self.schema.getDefaultValue("exampleKey5"), 1442244)
            
        except Exception,e:
            self.fail("test_getDefaultValue exception: " + str(e))
    
    def test_setDefaultValue(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.isAssignmentOptional('x'))
            self.assertTrue(schema.hasDefaultValue('x'))
            self.assertEqual(schema.getDefaultValue("x"), 5)
            schema.setDefaultValue("x", 10)
            self.assertEqual(schema.getDefaultValue("x"), 10)
        except Exception,e:
            self.fail("test_setDefaultValue exception: " + str(e))
    
    def test_getAllowedStates(self):
        try:
            allowedStates = self.schema.getAllowedStates("exampleKey3")
            self.assertEqual(allowedStates[0], "AllOk.Started")
            self.assertEqual(allowedStates[1], "AllOk.Stopped")
            self.assertEqual(self.schema.getAllowedStates("exampleKey3")[2], "AllOk.Run.On")
            self.assertEqual(self.schema.getAllowedStates("exampleKey3")[3], "NewState")
            
            self.assertEqual(self.schema.getAllowedStates("exampleKey7")[0], "Started")
            self.assertEqual(self.schema.getAllowedStates("exampleKey7")[1], "AllOk")
        except Exception,e:
            self.fail("test_getAllowedStates exception: " + str(e))
    
    def test_getUnit(self):
        try:
            self.assertEqual(self.schema.getUnit("exampleKey2"), Unit.METER)
            self.assertEqual(self.schema.getUnitName("exampleKey2"), "meter")
            self.assertEqual(self.schema.getUnitSymbol("exampleKey2"), "m")
        except Exception,e:
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
        except Exception,e:
            self.fail("test_setUnit exception: " + str(e))
        
    def test_getMetricPrefix(self):
        try:
            self.assertEqual(self.schema.getMetricPrefix("exampleKey2"), MetricPrefix.MILLI)
            self.assertEqual(self.schema.getMetricPrefixName("exampleKey2"), "milli")
            self.assertEqual(self.schema.getMetricPrefixSymbol("exampleKey2"), "m")
        except Exception,e:
            self.fail("test_getMetricPrefix exception: " + str(e))
    
    def test_setMetricPrefix(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getMetricPrefix("x"), MetricPrefix.MILLI)
            schema.setMetricPrefix("x", MetricPrefix.MICRO)
            self.assertEqual(schema.getMetricPrefix("x"), MICRO)
            self.assertEqual(schema.getMetricPrefixName("x"), "micro")
            self.assertEqual(schema.getMetricPrefixSymbol("x"), "u")
        except Exception,e:
            self.fail("test_setMetricPrefix exception: " + str(e))
    
    def test_getMinIncMaxInc(self):
        try:
            self.assertEqual(self.schema.getMinInc("exampleKey2"), 5)
            self.assertEqual(self.schema.getMinIncAs("exampleKey2", Types.STRING), "5")
            self.assertEqual(self.schema.getMaxInc("exampleKey2"), 25)
            self.assertEqual(self.schema.getMaxIncAs("exampleKey2", Types.STRING), "25")
        except Exception,e:
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
        except Exception,e:
            self.fail("test_setMinIncMaxInc exception: " + str(e))
    
    def test_getMinExcMaxExc(self):
        try:
            self.assertEqual(self.schema.getMinExc("exampleKey3"), 10)
            self.assertEqual(self.schema.getMinExc("exampleKey4"), -2.22)
        except Exception,e:
            self.fail("test_getMinExcMaxExc exception in getMinExc: " + str(e))
            
        try:    
            self.assertEqual(self.schema.getMaxExc("exampleKey3"), 20)
            self.assertEqual(self.schema.getMaxExc("exampleKey4"), 5.55)
        except Exception,e:
            self.fail("test_getMinExcMaxExc exception in getMaxExc: " + str(e))
        
        try:
            self.assertEqual(self.schema.getMinExcAs("exampleKey3", Types.STRING), "10")
            self.assertEqual(self.schema.getMinExcAs("exampleKey4", Types.STRING), "-2.220000000000000")
        except Exception,e:
            self.fail("test_getMinExcMaxExc exception in getMinExcAs: " + str(e))
        
        try:    
            self.assertEqual(self.schema.getMaxExcAs("exampleKey3", Types.STRING), "20")
            self.assertEqual(self.schema.getMaxExcAs("exampleKey4", Types.STRING), "5.550000000000000")
        except Exception,e:
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
        except Exception,e:
            self.fail("test_setMinExcMaxExc exception in getMinExc: " + str(e))

    def test_getWarnAlarmLowHigh(self):
        try:
            self.assertEqual(self.schema.getWarnLow("exampleKey5"), -10)
            self.assertEqual(self.schema.getWarnLow("exampleKey6"), -5.5)
            self.assertEqual(self.schema.getWarnLow("testPath2"), "d")
        except Exception,e:
            self.fail("test_getWarnAlarmLowHigh exception in getWarnLow: " + str(e))
        
        try:
            self.assertEqual(self.schema.getWarnHigh("exampleKey5"), 10)
            self.assertEqual(self.schema.getWarnHigh("exampleKey6"), 5.5)
            self.assertEqual(self.schema.getWarnHigh("testPath2"), "c")
        except Exception,e:
            self.fail("test_getWarnAlarmLowHigh exception in getWarnHigh: " + str(e))
           
        try:
            self.assertEqual(self.schema.getAlarmLow("exampleKey5"), -20)
            self.assertEqual(self.schema.getAlarmLow("exampleKey6"), -22.1)
            self.assertEqual(self.schema.getAlarmLow("testPath2"), "b")
        except Exception,e:
            self.fail("test_getWarnAlarmLowHigh exception in getAlarmLow: " + str(e))
        
        try:
            self.assertEqual(self.schema.getAlarmHigh("exampleKey5"), 20)
            self.assertEqual(self.schema.getAlarmHigh("exampleKey6"), 22.777)
            self.assertEqual(self.schema.getAlarmHigh("testPath2"), "a")
        except Exception,e:
            self.fail("test_getWarnAlarmLowHigh exception in getAlarmHigh: " + str(e))
    
    def test_getWarnAlarmLowHighAs(self):
        try:
            self.assertEqual(self.schema.getWarnLowAs("exampleKey5", Types.STRING), "-10")
            self.assertEqual(self.schema.getWarnLowAs("exampleKey6", Types.STRING), "-5.500000000000000")
        except Exception,e:
            self.fail("test_getWarnAlarmLowHighAs exception in getWarnLowAs: " + str(e))
        
        try:
            self.assertEqual(self.schema.getWarnHighAs("exampleKey5", Types.STRING), "10")
            self.assertEqual(self.schema.getWarnHighAs("exampleKey6", Types.STRING), "5.500000000000000")
        except Exception,e:
            self.fail("test_getWarnAlarmLowHighAs exception in getWarnHighAs: " + str(e))
           
        try:
            self.assertEqual(self.schema.getAlarmLowAs("exampleKey5", Types.STRING), "-20")
            self.assertEqual(self.schema.getAlarmLowAs("exampleKey6", Types.STRING), "-22.100000000000001")
        except Exception,e:
            self.fail("test_getWarnAlarmLowHighAs exception in getAlarmLowAs: " + str(e))
        
        try:
            self.assertEqual(self.schema.getAlarmHighAs("exampleKey5", Types.STRING), "20")
            self.assertEqual(self.schema.getAlarmHighAs("exampleKey6", Types.STRING), "22.777000000000001")
        except Exception,e:
            self.fail("test_getWarnAlarmLowHighAs exception in getAlarmHighAs: " + str(e))
            
    def test_hasWarnAlarm(self):
        try:
            self.assertTrue(self.schema.hasWarnLow("exampleKey5"))
            self.assertTrue(self.schema.hasWarnHigh("exampleKey5"))
            
            self.assertTrue(self.schema.hasWarnLow("exampleKey6"))
            self.assertTrue(self.schema.hasWarnHigh("exampleKey6"))
            self.assertTrue(self.schema.hasAlarmLow("exampleKey6"))
            self.assertTrue(self.schema.hasAlarmHigh("exampleKey6"))
            
            self.assertFalse(self.schema.hasAlarmHigh("exampleKey1"))
        except Exception,e:
            self.fail("test_hasWarnAlarm exception: " + str(e))         

    def test_vectorElement(self):
        try:
            self.assertEqual(self.schema.isAccessReadOnly("exampleKey7"), True)
            self.assertEqual(self.schema.hasDefaultValue("exampleKey7"), True)
            self.assertEqual(self.schema.hasDefaultValue("exampleKey10"), True)
            self.assertEqual(self.schema.hasDefaultValue("exampleKey11"), True)
            self.assertEqual(self.schema.hasDefaultValue("exampleKey12"), True)
            self.assertEqual(self.schema.hasDefaultValue("exampleKey14"), True)
            self.assertEqual(self.schema.hasDefaultValue("exampleKey15"), True)
 
            self.assertEqual(self.schema.getAlarmLow("exampleKey7"), [-1,-1,-1])
            self.assertEqual(self.schema.getAlarmHigh("exampleKey7"), [-2,2,-2])
            self.assertEqual(self.schema.getWarnLow("exampleKey7"), [0,0,0])
            self.assertEqual(self.schema.getWarnHigh("exampleKey7"), [10,20,30])
            
            self.assertEqual(self.schema.getAlarmLow("exampleKey8"), [-1.1,-2.2,-3.3])
            self.assertEqual(self.schema.getWarnHigh("exampleKey8"), [5.5, 7.7, 9.9])
            
            self.assertEqual(self.schema.getAlarmLow("exampleKey9"), ["a","b"])
            self.assertEqual(self.schema.getWarnHigh("exampleKey9"), ["c", "d"])
            
            self.assertEqual(self.schema.getDefaultValue("exampleKey10"), [10, 20, 30])

            self.assertEqual(self.schema.getDefaultValue("exampleKey12"), [1.1, -2.2, 3.3])
            
            self.assertEqual(self.schema.getDefaultValue("exampleKey11"), [10, 20, 30])
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey11", Types.STRING), "10,20,30")
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey11", Types.VECTOR_INT32), [10, 20, 30])
            
            self.assertEqual(self.schema.getDefaultValue("exampleKey14"), ["Hallo", "World", "Test"])
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey14", Types.STRING), "Hallo,World,Test")
 
            self.assertEqual(self.schema.getDefaultValue("exampleKey15"), ["word1", "word2", "test"])
            self.assertEqual(self.schema.getDefaultValueAs("exampleKey15", Types.STRING), "word1,word2,test")
 
            self.assertEqual(self.schema.getMinSize("exampleKey10"), 2)
            self.assertEqual(self.schema.getMaxSize("exampleKey10"), 7)
        except Exception,e:
            self.fail("test_vectorElement exception: " + str(e))  
        
    def test_getDisplayType(self):
        try:    
            self.assertEqual(self.schema.getDisplayType("testPath"), "fileOut")
            self.assertEqual(self.schema.getDisplayType("testPath2"), "fileIn")
            self.assertEqual(self.schema.getDisplayType("testPath3"), "directory")
        except Exception,e:
            self.fail("test_getDisplayType exception: " + str(e))  
    
    def test_setDisplayType(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertFalse(schema.hasDisplayType('y'))
            schema.setDisplayType('y', 'blabla')
            self.assertTrue(schema.hasDisplayType('y'))
            self.assertEqual(schema.getDisplayType("y"), "blabla")
        except Exception,e:
            self.fail("test_setDisplayType exception: " + str(e))  
    
    def test_isCommand(self):
        try:  
            self.assertTrue(self.schema.isCommand("slotTest"))
        except Exception,e:
            self.fail("test_isCommand exception: " + str(e))
            
    def test_isProperty(self):
        try:  
            self.assertFalse(self.schema.isProperty("slotTest"))
            self.assertTrue(self.schema.isProperty("testPath2"))
            
        except Exception,e:
            self.fail("test_isProperty exception: " + str(e))

     
    def test_hasArchivePolicy(self):
        try:  
            self.assertTrue(self.schema.hasArchivePolicy("exampleKey5"))
            self.assertTrue(self.schema.hasArchivePolicy("exampleKey6"))
            self.assertTrue(self.schema.hasArchivePolicy("exampleKey7"))
            self.assertTrue(self.schema.hasArchivePolicy("exampleKey8"))
        except Exception,e:
            self.fail("test_hasArchivePolicy exception: " + str(e))
   
   def test_getArchivePolicy(self):
        try:  
            self.assertEqual(self.schema.getArchivePolicy("exampleKey5"), ArchivePolicy.EVERY_EVENT)
            self.assertEqual(self.schema.getArchivePolicy("exampleKey6"), ArchivePolicy.EVERY_100MS)
            self.assertEqual(self.schema.getArchivePolicy("exampleKey7"), ArchivePolicy.EVERY_15S)
            self.assertEqual(self.schema.getArchivePolicy("exampleKey8"), ArchivePolicy.NO_ARCHIVING)
        except Exception,e:
            self.fail("test_getArchivePolicy exception: " + str(e))
   
   def test_setArchivePolicy(self):
        try:
            sch = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(self.sch.getArchivePolicy("a"), ArchivePolicy.EVERY_100MS)
            sch.setArchivePolicy('a', ArchivePolicy.EVERY_10MIN)
            self.assertEqual(self.sch.getArchivePolicy("a"), ArchivePolicy.EVERY_10MIN)
        except Exception,e:
            self.fail("test_setArchivePolicy exception: " + str(e))
     
        
    def test_perKeyFunctionality(self):
        try:
            keys = self.schema.getKeys()
            for key in keys:
                if key == "exampleKey1":
                    self.assertTrue(self.schema.hasAssignment(key))
                    self.assertTrue(self.schema.isAssignmentOptional(key))
                    self.assertTrue(self.schema.hasDefaultValue(key))
                    self.assertTrue(self.schema.hasAccessMode(key))
                    self.assertTrue(self.schema.isAccessReconfigurable(key))
                    self.assertTrue(self.schema.hasOptions(key))
                    self.assertTrue(self.schema.hasTags(key))
                    self.assertFalse(self.schema.hasUnit(key))
                    self.assertFalse(self.schema.hasMetricPrefix(key))
                    
                if key == "exampleKey2":
                    self.assertTrue(self.schema.hasDefaultValue(key))
                    self.assertTrue(self.schema.hasAccessMode(key))
                    self.assertTrue(self.schema.isAccessInitOnly(key))
                    self.assertTrue(self.schema.hasOptions(key))
                    self.assertTrue(self.schema.hasTags(key))
                    self.assertFalse(self.schema.hasAllowedStates(key))
                    self.assertTrue(self.schema.hasUnit(key))
                    self.assertTrue(self.schema.hasMetricPrefix(key))
                    self.assertTrue(self.schema.hasMinInc(key))
                    self.assertTrue(self.schema.hasMaxInc(key))
                    
                if key == "exampleKey3":
                    self.assertTrue(self.schema.hasAssignment(key))
                    self.assertTrue(self.schema.isAssignmentMandatory(key))
                    self.assertFalse(self.schema.hasDefaultValue(key))
                    self.assertFalse(self.schema.hasOptions(key))
                    self.assertTrue(self.schema.hasAllowedStates(key))
                    self.assertTrue(self.schema.hasMinExc(key))
                    self.assertTrue(self.schema.hasMaxExc(key))
                    
                if key == "exampleKey4":
                    self.assertFalse(self.schema.hasDefaultValue(key))
                    self.assertTrue(self.schema.isAssignmentInternal(key))
                    self.assertTrue(self.schema.hasAccessMode(key))
                    self.assertTrue(self.schema.isAccessInitOnly(key))
                    
                if key == "exampleKey5":
                    self.assertTrue(self.schema.hasDefaultValue(key))
                    self.assertTrue(self.schema.hasAssignment(key))
                    self.assertTrue(self.schema.isAssignmentOptional(key))
                    self.assertTrue(self.schema.hasAccessMode(key))
                    self.assertTrue(self.schema.isAccessReadOnly(key))   
                    
        except Exception,e:
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
            
        except Exception,e:
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
        #self.schema.help()


if __name__ == '__main__':
    unittest.main()

