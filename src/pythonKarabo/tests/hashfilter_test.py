# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest

from libkarathon import *
from karabo_decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from configurator import Configurator

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Base", "1.0")
class Base(object):
    def __init__(self, configuration):
        super(Base,self).__init__()
        

@KARABO_CLASSINFO("P1", "1.0")
class P1(Base):
    def __init__(self, configuration):
        super(P1,self).__init__(configuration)
        
    @staticmethod
    def expectedParameters(expected):
        
        e = STRING_ELEMENT(expected).key("a")
        e.description("a").displayedName("a")
        e.assignmentOptional().defaultValue("a value")
        e.tags("CY,CY,NC,JS,KW,NC").commit()

        e = STRING_ELEMENT(expected).key("b")
        e.tags("BH,CY")
        e.displayedName("Example key 1").description("Example key 1 description")
        e.options("Radio,Air Condition,Navigation", ",")
        e.assignmentOptional().defaultValue("Air Condition")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected).key("c").alias(10)
        e.tags("BH")
        e.displayedName("Example key 2").description("Example key 2 description")
        e.options("5, 25, 10")
        e.minInc(5).maxInc(25)
        e.unit(METER)
        e.metricPrefix(MILLI)
        e.assignmentOptional().defaultValue(5)
        e.init().commit()

        e = UINT32_ELEMENT(expected).key("d").alias(5.5)
        e.tags("CY,JS")
        e.displayedName("Example key 3").description("Example key 3 description")
        e.allowedStates("AllOk.Started, AllOk.Stopped, AllOk.Run.On, NewState") #TODO check
        e.minExc(10).maxExc(20).assignmentOptional().defaultValue(11)
        e.reconfigurable().commit()

        e = FLOAT_ELEMENT(expected).key("e").alias("exampleAlias4")
        e.tags("DB,NC,CY")
        e.displayedName("Example key 4").description("Example key 4 description")
        e.options("1.1100000   -2.22 5.55").assignmentOptional().defaultValue(1.11)
        e.commit()

        e = INT64_ELEMENT(expected).key("f").alias("exampleAlias5")
        e.tags("LM,DB")
        e.displayedName("Example key 5").description("Example key 5 description")
        e.assignmentOptional().defaultValue(5).commit()
        
        
@KARABO_CLASSINFO("P2", "1.0")
class P2(Base):
    def __init__(self, configuration):
        super(P2,self).__init__(configuration)
        
    @staticmethod
    def expectedParameters(expected):
        
        e = STRING_ELEMENT(expected).key("x")
        e.description("x").displayedName("x")
        e.assignmentOptional().defaultValue("a value")
        e.tags("LM,BH").commit()

        e = STRING_ELEMENT(expected).key("y")
        e.tags("CY")
        e.displayedName("Example key 1").description("Example key 1 description")
        e.options("Radio,Air Condition,Navigation", ",")
        e.assignmentOptional().defaultValue("Radio")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected).key("z").alias(10)
        e.tags("CY,LM,KW")
        e.displayedName("Example key 2").description("Example key 2 description")
        e.options("5, 25, 10")
        e.minInc(5).maxInc(25)
        e.unit(METER)
        e.metricPrefix(MILLI)
        e.assignmentOptional().defaultValue(10)
        e.init().commit()
        
        
@KARABO_CLASSINFO("P3", "1.0")
class P3(Base):
    def __init__(self, configuration):
        super(P3,self).__init__(configuration)
        
    @staticmethod
    def expectedParameters(expected):
        
        e = STRING_ELEMENT(expected).key("k")
        e.description("k").displayedName("k")
        e.assignmentOptional().defaultValue("k value")
        e.tags("LM").commit()

        e = STRING_ELEMENT(expected).key("l")
        e.tags("CY")
        e.displayedName("l").description("l")
        e.options("Radio,Air Condition,Navigation", ",")
        e.assignmentOptional().defaultValue("Navigation")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected).key("m").alias(10)
        e.tags("CY,DB,JE,BP,MK,PG,BF")
        e.displayedName("Example key 2").description("Example key 2 description")
        e.options("5, 25, 10")
        e.minInc(5).maxInc(25)
        e.unit(METER)
        e.metricPrefix(MILLI)
        e.assignmentOptional().defaultValue(25)
        e.init().commit()
        
        
@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("GraphicsRenderer2", "1.0")
class GraphicsRenderer2(object):
    def __init__(self, configuration):
        super(GraphicsRenderer2,self).__init__()
        
    @staticmethod
    def expectedParameters(expected):
        
        e = BOOL_ELEMENT(expected).key("antiAlias")
        e.tags("NC")
        e.displayedName("Use Anti-Aliasing").description("You may switch of for speed")
        e.assignmentOptional().defaultValue(True)
        e.init().advanced().commit()

        e = STRING_ELEMENT(expected).key("color")
        e.tags("KW")
        e.displayedName("Color").description("The default color for any shape")
        e.assignmentOptional().defaultValue("red")
        e.reconfigurable().commit()

        e = BOOL_ELEMENT(expected).key("bold")
        e.tags("LM")
        e.displayedName("Bold").description("Toggles bold painting")
        e.assignmentOptional().defaultValue(False)
        e.reconfigurable().commit()

        e = CHOICE_ELEMENT(expected).key("shapes")
        e.tags("DB")
        e.assignmentOptional().defaultValue("rectangle")
        e.commit()

        e = NODE_ELEMENT(expected).key("shapes.circle")
        e.tags("JS")
        e.displayedName("Circle").description("A circle")
        e.commit()

        e = FLOAT_ELEMENT(expected).key("shapes.circle.radius")
        e.description("The radius of the circle").displayedName("Radius")
        e.tags("NC,KW")
        e.minExc(0).maxExc(100)
        e.unit(METER)
        e.metricPrefix(MILLI)
        e.assignmentOptional().defaultValue(10)
        e.init().commit()

        e = NODE_ELEMENT(expected).key("shapes.rectangle")
        e.tags("BH, KW , CY")
        e.displayedName("Rectangle").description("A rectangle")
        e.commit()

        e = FLOAT_ELEMENT(expected).key("shapes.rectangle.b")
        e.tags("JS")
        e.description("Rectangle side - b").displayedName("Side B")
        e.assignmentOptional().defaultValue(10)
        e.init().commit()

        e = FLOAT_ELEMENT(expected).key("shapes.rectangle.c")
        e.tags("LM,JS")
        e.description("Rectangle side - c").displayedName("Side C")
        e.assignmentOptional().defaultValue(10)
        e.init().commit()

        e = NODE_ELEMENT(expected).key("shapes.triangle")
        e.displayedName("triangle").description("A triangle (Node element containing no other elements)")
        e.commit()

        e = NODE_ELEMENT(expected).key("letter")
        e.displayedName("Letter").description("Letter")
        e.appendParametersOf(P1)
        e.commit()

        e = LIST_ELEMENT(expected).key("chars")
        e.displayedName("characters").description("Characters")
        e.tags("LM")
        e.appendNodesOfConfigurationBase(Base)
        e.assignmentOptional().defaultValueFromString("P2,P3")
        e.commit()

    
        
class  HashfilterTestCase(unittest.TestCase):

    def test_hashfilter(self):
        try:
            schema = Configurator(GraphicsRenderer2).getSchema("GraphicsRenderer2")
            validator = Validator()
            config = validator.validate(schema, Hash())
            result = HashFilter.byTag(schema, config, "KW;KW,BH", ",;")

            self.assertFalse("antiAlias" in result)
            self.assertTrue("color" in result)
            self.assertFalse("bold" in result)
            self.assertTrue("shapes" in result)
            self.assertTrue("shapes.rectangle" in result)
            self.assertTrue("shapes.rectangle.b" in result)
            self.assertTrue("shapes.rectangle.c" in result)
            self.assertTrue("letter" in result)
            self.assertTrue("letter.a" in result)
            self.assertTrue("letter.b" in result)
            self.assertTrue("letter.c" in result)
            self.assertFalse("letter.d" in result)
            self.assertFalse("letter.e" in result)
            self.assertFalse("letter.f" in result)
            self.assertTrue("chars" in result)
            self.assertTrue("chars[0]" in result)
            self.assertTrue("chars[0].P2" in result)
            self.assertTrue("chars[0].P2.x" in result)
            self.assertFalse("chars[0].P2.y" in result)
            self.assertTrue("chars[0].P2.z" in result)
            self.assertTrue("chars[1]" in result)
            self.assertFalse("chars[1].P3" in result)
            self.assertFalse("chars[1].P3.k" in result)
            self.assertFalse("chars[1].P3.l" in result)
            self.assertFalse("chars[1].P3.m" in result)
            
        except Exception, e:
            self.fail("test_hashfilter exception group 1: " + str(e))

        try:
            result = HashFilter.byTag(schema, config, "JS", ",;")
            
            self.assertFalse("antiAlias" in result)
            self.assertFalse("color" in result)
            self.assertFalse("bold" in result)
            
        except Exception, e:
            self.fail("test_hashfilter exception group 1: " + str(e))

            
if __name__ == '__main__':
    unittest.main()

