__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Apr 14, 2013 12:07:53 PM$"

import unittest
from karabo.karathon import *
from karabo.decorators import *

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Shape", "1.0")
class Shape(object):
    
    def __init__(self, configuration):
        self.configuration = configuration
        #print "Shape.__init__"
        
    @staticmethod
    def expectedParameters(expected):
        (
        BOOL_ELEMENT(expected).key("shadowEnabled")
         .description("Shadow enabled")
         .displayedName("Shadow")
         .assignmentOptional().defaultValue(False)
         .init()
         .commit(),
         )
        
    def getConfiguration(self):
        return self.configuration
        
    def draw(self):
        pass
    
    
@KARABO_CLASSINFO("Circle", "1.0")    
class Circle(Shape):
    
    def __init__(self, configuration):
        super(Circle, self).__init__(configuration)
        #print "Circle.__init__"
        
    @staticmethod
    def expectedParameters(expected):
        (
        DOUBLE_ELEMENT(expected).key("radius").alias(1)
         .description("The radius of the circle")
         .displayedName("Radius")
         .minExc(0).maxExc(100)
         .unit(METER)
         .metricPrefix(MILLI)
         .assignmentOptional().defaultValue(10)
         .init()
         .commit(),
        )
        
    def draw(self):
        return self.__class__.__name__


'''
Editable Circle
'''
@KARABO_CLASSINFO("EditableCircle", "1.0")    
class EditableCircle(Circle):
    
    def __init__(self, configuration):
        super(EditableCircle, self).__init__(configuration)
        #print "EditableCircle.__init__"
        
    @staticmethod
    def expectedParameters(expected):
        (
        OVERWRITE_ELEMENT(expected)
         .key("radius")
         .setNowReconfigurable()
         .commit(),
         )
        
    def draw(self):
        return self.__class__.__name__

'''
Rectangle
'''
@KARABO_CLASSINFO("Rectangle", "1.0")    
class Rectangle(Shape):
    
    def __init__(self, configuration):
        super(Rectangle, self).__init__(configuration)
        #print "Rectangle.__init__"
    
    @staticmethod
    def expectedParameters(expected):
        (
        DOUBLE_ELEMENT(expected).key("a").alias(1)
         .description("Length of a")
         .displayedName("A")
         .minExc(0).maxExc(100)
         .unit(Unit.METER)
         .metricPrefix(MetricPrefix.MILLI)
         .assignmentOptional().defaultValue(10)
         .init()
         .commit(),
        
        DOUBLE_ELEMENT(expected).key("b").alias(1)
         .description("Length of b")
         .displayedName("B")
         .minExc(0).maxExc(100)
         .unit(Unit.METER)
         .metricPrefix(MetricPrefix.MILLI)
         .assignmentOptional().defaultValue(10)
         .init()
         .commit(),
        )

    def draw(self):
        return self.__class__.__name
    

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("GraphicsRenderer", "1.0")    
class GraphicsRenderer(object):
    
    def __init__(self, input):
        shape = Shape.createChoice("shapes", input)
        #assert input["version"] == "1.4.7"
        if "shapes.Circle" in input:
            assert shape.draw() == "Circle"
        
    @staticmethod
    def expectedParameters(expected):
        (
        BOOL_ELEMENT(expected).key("antiAlias")
         .tags("prop")
         .displayedName("Use Anti-Aliasing")
         .description("You may switch of for speed")
         .assignmentOptional().defaultValue(True)
         .init()
         .expertAccess()
         .commit(),

        STRING_ELEMENT(expected).key("color")
         .tags("prop")
         .displayedName("Color")
         .options("red,green,blue,orange,black")
         .description("The default color for any shape")
         .assignmentOptional().defaultValue("red")
         .reconfigurable()
         .commit(),

        BOOL_ELEMENT(expected).key("bold")
         .tags("prop")
         .displayedName("Bold")
         .description("Toggles bold painting")
         .assignmentOptional().defaultValue(False)
         .reconfigurable()
         .commit(),

        CHOICE_ELEMENT(expected).key("shapes")
         .description("Some shapes")
         .displayedName("Shapes")
         .appendNodesOfConfigurationBase(Shape)
         .assignmentOptional().defaultValue("Rectangle")
         .commit(),
        )
        
        
@KARABO_CLASSINFO("GraphicsRenderer1", "1.0")    
class GraphicsRenderer1(object):
    
    @staticmethod
    def expectedParameters(expected):
        (
        BOOL_ELEMENT(expected).key("antiAlias")
         .tags("prop")
         .displayedName("Use Anti-Aliasing")
         .description("You may switch of for speed")
         .assignmentOptional().defaultValue(True)
         .init()
         .expertAccess()
         .commit(),

        STRING_ELEMENT(expected).key("color")
         .tags("prop")
         .displayedName("Color")
         .description("The default color for any shape")
         .assignmentOptional().defaultValue("red")
         .reconfigurable()
         .commit(),

        BOOL_ELEMENT(expected).key("bold")
         .tags("prop")
         .displayedName("Bold")
         .description("Toggles bold painting")
         .assignmentOptional().defaultValue(False)
         .reconfigurable()
         .commit(),

        CHOICE_ELEMENT(expected).key("shapes")
         .assignmentOptional().defaultValue("circle")
         .commit(),

        NODE_ELEMENT(expected).key("shapes.circle")
         .tags("shape")
         .displayedName("Circle")
         .description("A circle")
         .appendParametersOf(Circle)
         .commit(),

        NODE_ELEMENT(expected).key("shapes.rectangle")
         .tags("shape")
         .displayedName("Rectangle")
         .description("A rectangle")
         .commit(),

        DOUBLE_ELEMENT(expected).key("shapes.rectangle.b")
         .description("Rectangle side - b")
         .displayedName("Side B")
         .assignmentOptional().defaultValue(10)
         .init()
         .commit(),

        DOUBLE_ELEMENT(expected).key("shapes.rectangle.c")
         .description("Rectangle side - c")
         .displayedName("Side C")
         .assignmentOptional().defaultValue(10)
         .init()
         .commit(),

        NODE_ELEMENT(expected).key("triangle")
         .displayedName("triangle")
         .description("A triangle (Node element containing no other elements)")
         .commit(),
        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("TestStruct1", "1.0")    
class TestStruct1(object):
    
    @staticmethod
    def expectedParameters(expected):
        (
        STRING_ELEMENT(expected).key("exampleKey1")
                .tags("hardware, poll")
                .displayedName("Example key 1")
                .description("Example key 1 description")
                .options("Radio,Air Condition,Navigation", ",")
                .assignmentOptional().defaultValue("Navigation")
                .reconfigurable()
                .userAccess()
                .commit()
                ,
        INT32_ELEMENT(expected).key("exampleKey2").alias(10)
                .tags("hardware, poll")
                .displayedName("Example key 2")
                .description("Example key 2 description")
                .options("5, 25, 10")
                .minInc(5).maxInc(25)
                .unit(METER)
                .metricPrefix(MILLI)
                .assignmentOptional().defaultValue(10)
                .init()
                .operatorAccess()
                .commit()
                ,
        UINT32_ELEMENT(expected).key("exampleKey3").alias(5.5)
                .tags("hardware, set")
                .displayedName("Example key 3")
                .description("Example key 3 description")
                .allowedStates("AllOk.Started, AllOk.Stopped, AllOk.Run.On, NewState")
                .minExc(10).maxExc(20)
                .assignmentMandatory()
                .expertAccess()
                .reconfigurable()
                .commit()
                ,
        DOUBLE_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
                .tags("software")
                .displayedName("Example key 4")
                .description("Example key 4 description")
                .options("1.11     -2.22 5.55")
                .minExc(-2.22).maxExc(5.55)
                .assignmentInternal().noDefaultValue()
                .adminAccess()
                .commit()
                ,
        INT64_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
                .tags("h/w; d.m.y", ";")
                .displayedName("Example key 5")
                .description("Example key 5 description")
                .readOnly().initialValue(1442244).warnLow(-10).warnHigh(10).alarmLow(-20).alarmHigh(20).archivePolicy(EVERY_EVENT)
                .commit()
                ,
        DOUBLE_ELEMENT(expected).key("exampleKey6").alias([0x00123456, 0x0000aacc])
                .displayedName("Example key 6")
                .description("Example key 6 description")
                .readOnly().initialValue(1.11)
                .alarmLow(-22.1).alarmHigh(22.777)
                .warnLow(-5.5).warnHigh(5.5).archivePolicy(EVERY_100MS)
                .commit()
                ,
        VECTOR_INT32_ELEMENT(expected).key("exampleKey7")
                .displayedName("Example key 7")
                .allowedStates("Started, AllOk")
                .readOnly().initialValue([1,2,3])
                .alarmLow([-1,-1,-1]).alarmHigh([-2,2,-2])
                .warnLow([0,0,0]).warnHigh([10,20,30]).archivePolicy(EVERY_1S)
                .commit()
                ,
        VECTOR_DOUBLE_ELEMENT(expected).key("exampleKey8")
                .readOnly().initialValue([1.1, 2.2, 3.3])
                .alarmLow([-1.1,-2.2,-3.3])
                .warnHigh([5.5, 7.7, 9.9]).archivePolicy(NO_ARCHIVING)
                .commit()
                ,
        VECTOR_STRING_ELEMENT(expected).key("exampleKey9")
                .readOnly().initialValue(["Hallo", "World"])
                .alarmLow(["a","b"]).warnHigh(["c", "d"])
                .commit()
                ,
        VECTOR_INT32_ELEMENT(expected).key("vectInt")
                .readOnly().alarmLow([1,2,2])
                .commit()
                ,
        VECTOR_INT32_ELEMENT(expected).key("exampleKey10")
                .displayedName("Example key 10")
                .minSize(2)
                .maxSize(7)
                .assignmentOptional().defaultValueFromString("10, 20, 30")
                .reconfigurable()
                .commit()
                ,
        VECTOR_INT32_ELEMENT(expected).key("exampleKey11")
                .displayedName("Example key 11")
                .assignmentOptional().defaultValue([10,20,30])
                .observerAccess()
                .reconfigurable()
                .commit()
                ,
        VECTOR_DOUBLE_ELEMENT(expected).key("exampleKey12")
                .assignmentOptional().defaultValueFromString("1.1, -2.2, 3.3")
                .reconfigurable()
                .commit()
                ,
        VECTOR_STRING_ELEMENT(expected).key("exampleKey14")
                .assignmentOptional().defaultValue(["Hallo", "World", "Test"])
                .reconfigurable()
                .commit()
                ,
        VECTOR_STRING_ELEMENT(expected).key("exampleKey15")
                .assignmentOptional().defaultValueFromString("word1, word2, test")
                .reconfigurable()
                .commit()
                ,
        INT32_ELEMENT(expected).key("exampleIntKey")
                .assignmentOptional().defaultValueFromString("20")
                .reconfigurable()
                .commit()
                ,
#        UINT32_ELEMENT(expected).key("exampleBitsKey1")
#                .tags("hardware")
#                .displayedName("Example bits key 1")
#                .description("Example bits key 1 description")
#                .reconfigurable()
#                .bin("0:inError, 1:busy, 5:HVOn, 8:CrateOn")
#                .assignmentOptional().defaultValue(0xdeadbeef)
#                .commit()
#                ,
#        UINT64_ELEMENT(expected).key("exampleBitsKey2")
#                .tags("hardware")
#                .displayedName("Example bits key 2")
#                .description("Example bits key 2 description")
#                .reconfigurable()
#                .bin("10:In Error, 21:Busy, 35:HV On, 55:Crate On")
#                .assignmentOptional().defaultValue(0xdeadbeefdeadface)
#                .commit()
#                ,
#        UINT8_ELEMENT(expected).key("exampleBitsKey3")
#                .tags("hardware")
#                .displayedName("Example bits key 3")
#                .description("Example bits key 3 description")
#                .reconfigurable()
#                .bin()
#                .assignmentOptional().defaultValue(0xbeef)
#                .commit()
#                ,
#        UINT32_ELEMENT(expected).key("exampleBitsKey4")
#                .tags("hardware")
#                .displayedName("Example bits key 4")
#                .description("Example bits key 4 description")
#                .reconfigurable()
#                .hex()
#                .assignmentOptional().defaultValue(0xbeefface)
#                .commit()
#                ,

        UINT32_ELEMENT(expected).key("exampleBitsKey1")
                .tags("hardware")
                .displayedName("Example bits key 1")
                .description("Example bits key 1 description")
                .reconfigurable()
                .bin()
                .assignmentOptional().defaultValue(0xdeadbeef)
                .commit()
                ,
        UINT64_ELEMENT(expected).key("exampleBitsKey2")
                .tags("hardware")
                .displayedName("Example bits key 2")
                .description("Example bits key 2 description")
                .reconfigurable()
                .bin("10:In Error, 21:Busy, 35:HV On, 55:Crate On")
                .assignmentOptional().defaultValue(0xdeadbeefdeadface)
                .commit()
                ,
        UINT32_ELEMENT(expected).key("exampleBitsKey3")
                .tags("hardware")
                .displayedName("Example bits key 3")
                .description("Example bits key 3 description")
                .reconfigurable()
                .oct()
                .assignmentOptional().defaultValue(0xbeefface)
                .commit()
                ,
        UINT32_ELEMENT(expected).key("exampleBitsKey4")
                .tags("hardware")
                .displayedName("Example bits key 4")
                .description("Example bits key 4 description")
                .reconfigurable()
                .hex()
                .assignmentOptional().defaultValue(0xbeefface)
                .commit()
                ,
        PATH_ELEMENT(expected).key("testPath")
                .alias(5)
                .displayedName("Filename")
                .isOutputFile()
                .options("file1, file2")
                .assignmentOptional().defaultValue("karabo.log")
                .reconfigurable()
                .commit()
                ,
        PATH_ELEMENT(expected).key("testPath2")
                .isInputFile()
                .readOnly().alarmHigh("a").alarmLow("b").warnHigh("c").warnLow("d")
                .commit()
                ,
        PATH_ELEMENT(expected).key("testPath3")
                .isDirectory()
                .assignmentMandatory()
                .commit()
                ,
        SLOT_ELEMENT(expected).key("slotTest")
                .displayedName("Reset")
                .description("Test slot element")
                .allowedStates("Started, Stopped, Reset")
                .commit()
                ,
        IMAGE_ELEMENT(expected).key("myImageElement")
                .displayedName("myImage")
                .description("Image Element")
                .operatorAccess()
                .commit()
                ,
        )
        
@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("SomeClassId", "1.0")
class SomeClass(object):
    
    def __init__(self, configuration):
        super(SomeClass,self).__init__()
        
    @staticmethod
    def expectedParameters(expected):
        (
        INT32_ELEMENT(expected).key("x").alias(10)
         .tags("IK,BH")
         .displayedName("Xkey").description("Example of X key description")
         .options("5, 25, 10")
         .minInc(5).maxInc(25).unit(AMPERE).metricPrefix(MILLI)
         .assignmentOptional().defaultValue(5)
         .init().expertAccess().commit()
         , 
        INT32_ELEMENT(expected).key("y").alias('bla')
         .tags("CY")
         .displayedName("Ykey").description("Example of Y key description")
         .options("5, 25, 10")
         .minExc(0).maxExc(29).unit(METER).metricPrefix(CENTI)
         .assignmentOptional().defaultValue(10)
         .init().commit()
         ,
        DOUBLE_ELEMENT(expected).key("a")
         .readOnly().initialValue(1.11).alarmLow(-22.1).alarmHigh(22.777).warnLow(-5.5).warnHigh(5.5).archivePolicy(EVERY_100MS)
         .commit()
         ,
        VECTOR_INT32_ELEMENT(expected).key("somelist")
                 .displayedName("Ykey").description("Example of Y key description")
                 .reconfigurable()
                 .assignmentOptional().defaultValue([])
                 .commit()
                 ,
        IMAGE_ELEMENT(expected).key("myImageElement")
                .displayedName("myImage")
                .description("Image Element")
                .operatorAccess()
                .commit()
                ,        
        )

    
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
        (
        STRING_ELEMENT(expected).key("a")
         .description("a").displayedName("a")
         .assignmentOptional().defaultValue("a value")
         .tags("CY,CY,NC,JS,KW,NC").commit(),

        STRING_ELEMENT(expected).key("b")
         .tags("BH,CY")
         .displayedName("Example key 1").description("Example key 1 description")
         .options("Radio,Air Condition,Navigation", ",")
         .assignmentOptional().defaultValue("Air Condition")
         .reconfigurable().commit(),

        INT32_ELEMENT(expected).key("c").alias(10)
         .tags("BH")
         .displayedName("Example key 2").description("Example key 2 description")
         .options("5, 25, 10")
         .minInc(5).maxInc(25).unit(METER).metricPrefix(MILLI)
         .assignmentOptional().defaultValue(5)
         .init().commit(),

        UINT32_ELEMENT(expected).key("d").alias(5.5)
         .tags("CY,JS")
         .displayedName("Example key 3").description("Example key 3 description")
         .allowedStates("AllOk.Started, AllOk.Stopped, AllOk.Run.On, NewState") #TODO check
         .minExc(10).maxExc(20).assignmentOptional().defaultValue(11)
         .reconfigurable().commit(),

        FLOAT_ELEMENT(expected).key("e").alias("exampleAlias4")
         .tags("DB,NC,CY")
         .displayedName("Example key 4").description("Example key 4 description")
         .options("1.1100000   -2.22 5.55").assignmentOptional().defaultValue(1.11)
         .commit(),

        INT64_ELEMENT(expected).key("f").alias("exampleAlias5")
         .tags("LM,DB")
         .displayedName("Example key 5").description("Example key 5 description")
         .assignmentOptional().defaultValue(5).commit(),
        )
        
        
@KARABO_CLASSINFO("P2", "1.0")
class P2(Base):
    def __init__(self, configuration):
        super(P2,self).__init__(configuration)
        
    @staticmethod
    def expectedParameters(expected):
        (
        STRING_ELEMENT(expected).key("x")
         .description("x").displayedName("x")
         .assignmentOptional().defaultValue("a value")
         .tags("LM,BH").commit(),

        STRING_ELEMENT(expected).key("y")
         .tags("CY")
         .displayedName("Example key 1").description("Example key 1 description")
         .options("Radio,Air Condition,Navigation", ",")
         .assignmentOptional().defaultValue("Radio")
         .reconfigurable().commit(),

        INT32_ELEMENT(expected).key("z").alias(10)
         .tags("CY,LM,KW")
         .displayedName("Example key 2").description("Example key 2 description")
         .options("5, 25, 10")
         .minInc(5).maxInc(25).unit(AMPERE).metricPrefix(MILLI)
         .assignmentOptional().defaultValue(10)
         .init().commit(),
        )
        
@KARABO_CLASSINFO("P3", "1.0")
class P3(Base):
    def __init__(self, configuration):
        super(P3,self).__init__(configuration)
        
    @staticmethod
    def expectedParameters(expected):
        (
        STRING_ELEMENT(expected).key("k")
         .description("k").displayedName("k")
         .assignmentOptional().defaultValue("k value")
         .tags("LM").commit(),

        STRING_ELEMENT(expected).key("l")
         .tags("CY")
         .displayedName("l").description("l")
         .options("Radio,Air Condition,Navigation", ",")
         .assignmentOptional().defaultValue("Navigation")
         .reconfigurable().commit(),

        INT32_ELEMENT(expected).key("m").alias(10)
         .tags("CY,DB,JE,BP,MK,PG,BF")
         .displayedName("Example key 2").description("Example key 2 description")
         .options("5, 25, 10")
         .minInc(5).maxInc(25).unit(METER).metricPrefix(MILLI)
         .assignmentOptional().defaultValue(25)
         .init().commit(),
        )
        
@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("GraphicsRenderer2", "1.0")
class GraphicsRenderer2(object):
    def __init__(self, configuration):
        super(GraphicsRenderer2,self).__init__()
        
    @staticmethod
    def expectedParameters(expected):
        (
        BOOL_ELEMENT(expected).key("antiAlias")
         .tags("NC")
         .displayedName("Use Anti-Aliasing").description("You may switch of for speed")
         .assignmentOptional().defaultValue(True)
         .init().expertAccess().commit(),

        STRING_ELEMENT(expected).key("color")
         .tags("KW")
         .displayedName("Color").description("The default color for any shape")
         .assignmentOptional().defaultValue("red")
         .reconfigurable().commit(),

        BOOL_ELEMENT(expected).key("bold")
         .tags("LM")
         .displayedName("Bold").description("Toggles bold painting")
         .assignmentOptional().defaultValue(False)
         .reconfigurable().commit(),

        CHOICE_ELEMENT(expected).key("shapes")
         .tags("DB")
         .assignmentOptional().defaultValue("rectangle")
         .commit(),

        NODE_ELEMENT(expected).key("shapes.circle")
         .tags("JS")
         .displayedName("Circle").description("A circle")
         .commit(),

        FLOAT_ELEMENT(expected).key("shapes.circle.radius")
         .description("The radius of the circle").displayedName("Radius")
         .tags("NC,KW")
         .minExc(0).maxExc(100).unit(METER).metricPrefix(MILLI)
         .assignmentOptional().defaultValue(10)
         .init().commit(),

        NODE_ELEMENT(expected).key("shapes.rectangle")
         .tags("BH, KW , CY")
         .displayedName("Rectangle").description("A rectangle")
         .commit(),

        FLOAT_ELEMENT(expected).key("shapes.rectangle.b")
         .tags("JS")
         .description("Rectangle side - b").displayedName("Side B")
         .assignmentOptional().defaultValue(10)
         .init().commit(),

        FLOAT_ELEMENT(expected).key("shapes.rectangle.c")
         .tags("LM,JS")
         .description("Rectangle side - c").displayedName("Side C")
         .assignmentOptional().defaultValue(10)
         .init().commit(),

        NODE_ELEMENT(expected).key("shapes.triangle")
         .displayedName("triangle").description("A triangle (Node element containing no other elements)")
         .commit(),

        NODE_ELEMENT(expected).key("letter")
         .displayedName("Letter").description("Letter")
         .appendParametersOf(P1)
         .commit(),

        LIST_ELEMENT(expected).key("chars")
         .displayedName("characters").description("Characters")
         .tags("LM")
         .appendNodesOfConfigurationBase(Base)
         .assignmentOptional().defaultValueFromString("P2,P3")
         .commit(),
        )

