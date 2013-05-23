__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Apr 14, 2013 12:07:53 PM$"

import unittest
from libkarathon import *
from karabo_decorators import *

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Shape", "1.0")
class Shape(object):
    
    def __init__(self, configuration):
        self.configuration = configuration
        #print "Shape.__init__"
        
    @staticmethod
    def expectedParameters(expected):

        e = BOOL_ELEMENT(expected).key("shadowEnabled")
        e.description("Shadow enabled")
        e.displayedName("Shadow")
        e.assignmentOptional().defaultValue(False)
        e.init()
        e.commit()
        
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
        
        e = DOUBLE_ELEMENT(expected).key("radius").alias(1)
        e.description("The radius of the circle")
        e.displayedName("Radius")
        e.minExc(0).maxExc(100)
        e.unit(METER)
        e.metricPrefix(MILLI)
        e.assignmentOptional().defaultValue(10)
        e.init()
        e.commit()
        
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
        
        e = OVERWRITE_ELEMENT(expected)
        e.key("radius")
        e.setNowReconfigurable()
        e.commit()
        
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
        
        e = DOUBLE_ELEMENT(expected).key("a").alias(1)
        e.description("Length of a")
        e.displayedName("A")
        e.minExc(0).maxExc(100)
        e.unit(Unit.METER)
        e.metricPrefix(MetricPrefix.MILLI)
        e.assignmentOptional().defaultValue(10)
        e.init()
        e.commit()

        e = DOUBLE_ELEMENT(expected).key("b").alias(1)
        e.description("Length of b")
        e.displayedName("B")
        e.minExc(0).maxExc(100)
        e.unit(Unit.METER)
        e.metricPrefix(MetricPrefix.MILLI)
        e.assignmentOptional().defaultValue(10)
        e.init()
        e.commit()

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
        
        e = BOOL_ELEMENT(expected).key("antiAlias")
        e.tags("prop")
        e.displayedName("Use Anti-Aliasing")
        e.description("You may switch of for speed")
        e.assignmentOptional().defaultValue(True)
        e.init()
        e.advanced()
        e.commit()

        e = STRING_ELEMENT(expected).key("color")
        e.tags("prop")
        e.displayedName("Color")
        e.options("red,green,blue,orange,black")
        e.description("The default color for any shape")
        e.assignmentOptional().defaultValue("red")
        e.reconfigurable
        e.commit()

        e = BOOL_ELEMENT(expected).key("bold")
        e.tags("prop")
        e.displayedName("Bold")
        e.description("Toggles bold painting")
        e.assignmentOptional().defaultValue(False)
        e.reconfigurable()
        e.commit()

        e = CHOICE_ELEMENT(expected).key("shapes")
        e.description("Some shapes")
        e.displayedName("Shapes")
        e.appendNodesOfConfigurationBase(Shape)
        e.assignmentOptional().defaultValue("Rectangle")
        e.commit()
        
        
@KARABO_CLASSINFO("GraphicsRenderer1", "1.0")    
class GraphicsRenderer1(object):
    
    @staticmethod
    def expectedParameters(expected):
        
        e = BOOL_ELEMENT(expected).key("antiAlias")
        e.tags("prop")
        e.displayedName("Use Anti-Aliasing")
        e.description("You may switch of for speed")
        e.assignmentOptional().defaultValue(True)
        e.init()
        e.advanced()
        e.commit()

        e = STRING_ELEMENT(expected).key("color")
        e.tags("prop")
        e.displayedName("Color")
        e.description("The default color for any shape")
        e.assignmentOptional().defaultValue("red")
        e.reconfigurable()
        e.commit()

        e = BOOL_ELEMENT(expected).key("bold")
        e.tags("prop")
        e.displayedName("Bold")
        e.description("Toggles bold painting")
        e.assignmentOptional().defaultValue(False)
        e.reconfigurable()
        e.commit()

        e = CHOICE_ELEMENT(expected).key("shapes")
        e.assignmentOptional().defaultValue("circle")
        e.commit()

        e = NODE_ELEMENT(expected).key("shapes.circle")
        e.tags("shape")
        e.displayedName("Circle")
        e.description("A circle")
        e.appendParametersOf(Circle)
        e.commit()

        e = NODE_ELEMENT(expected).key("shapes.rectangle")
        e.tags("shape")
        e.displayedName("Rectangle")
        e.description("A rectangle")
        e.commit()

        e = DOUBLE_ELEMENT(expected).key("shapes.rectangle.b")
        e.description("Rectangle side - b")
        e.displayedName("Side B")
        e.assignmentOptional().defaultValue(10)
        e.init()
        e.commit()

        e = DOUBLE_ELEMENT(expected).key("shapes.rectangle.c")
        e.description("Rectangle side - c")
        e.displayedName("Side C")
        e.assignmentOptional().defaultValue(10)
        e.init()
        e.commit()

        e = NODE_ELEMENT(expected).key("triangle")
        e.displayedName("triangle")
        e.description("A triangle (Node element containing no other elements)")
        e.commit()


@KARABO_CLASSINFO("TestStruct1", "1.0")    
class TestStruct1(object):
    
    @staticmethod
    def expectedParameters(expected):
        
        e = STRING_ELEMENT(expected).key("exampleKey1")
        e.tags("hardware, poll")
        e.displayedName("Example key 1")
        e.description("Example key 1 description")
        e.options("Radio,Air Condition,Navigation", ",")
        e.assignmentOptional().defaultValue("Navigation")
        e.reconfigurable()
        e.commit()

        e = INT32_ELEMENT(expected).key("exampleKey2").alias(10)
        e.tags("hardware, poll")
        e.displayedName("Example key 2")
        e.description("Example key 2 description")
        e.options("5, 25, 10")
        e.minInc(5).maxInc(25)
        e.unit(METER)
        e.metricPrefix(MILLI)
        e.assignmentOptional().defaultValue(10)
        e.init()
        e.commit()

        e = UINT32_ELEMENT(expected).key("exampleKey3").alias(5.5)
        e.tags("hardware, set")
        e.displayedName("Example key 3")
        e.description("Example key 3 description")
        e.allowedStates("AllOk.Started, AllOk.Stopped, AllOk.Run.On, NewState")
        e.minExc(10).maxExc(20)
        e.assignmentMandatory()
        e.reconfigurable()
        e.commit()

        e = DOUBLE_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
        e.tags("software")
        e.displayedName("Example key 4")
        e.description("Example key 4 description")
        e.options("1.11     -2.22 5.55")
        e.minExc(-2.22).maxExc(5.55)
        e.assignmentInternal().noDefaultValue()
        e.commit()

        e = INT64_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
        e.tags("h/w; d.m.y", ";")
        e.displayedName("Example key 5")
        e.description("Example key 5 description")
        e.readOnly().initialValue(1442244).warnLow(-10).warnHigh(10).alarmLow(-20).alarmHigh(20)
        e.commit()
                    
        e = DOUBLE_ELEMENT(expected).key("exampleKey6")
        e.displayedName("Example key 6")
        e.description("Example key 6 description")
        e.readOnly().initialValue(1.11).alarmLow(-22.1).alarmHigh(22.777).warnLow(-5.5).warnHigh(5.5)
        e.commit()
        
        e = VECTOR_INT32_ELEMENT(expected).key("exampleKey7")
        e.displayedName("Example key 7")
        e.allowedStates("Started, AllOk")
        e.readOnly().initialValue([1,2,3]).alarmLow([-1,-1,-1]).alarmHigh([-2,2,-2]).warnLow([0,0,0]).warnHigh([10,20,30])
        e.commit()
        
        e = VECTOR_DOUBLE_ELEMENT(expected).key("exampleKey8")
        e.readOnly().initialValue([1.1, 2.2, 3.3]).alarmLow([-1.1,-2.2,-3.3]).warnHigh([5.5, 7.7, 9.9]).commit()
        
        e = VECTOR_STRING_ELEMENT(expected).key("exampleKey9")
        e.readOnly().initialValue(["Hallo", "World"]).alarmLow(["a","b"]).warnHigh(["c", "d"])
        e.commit()
        
        e = VECTOR_INT32_ELEMENT(expected).key("vectInt")
        e.readOnly().alarmLow([1,2,2])
        e.commit()
        
        e = VECTOR_INT32_ELEMENT(expected).key("exampleKey10")
        e.displayedName("Example key 10")
        e.minSize(2)
        e.maxSize(7)
        e.assignmentOptional().defaultValueFromString("10, 20, 30")
        e.reconfigurable()
        e.commit()
        
        e = VECTOR_INT32_ELEMENT(expected).key("exampleKey11")
        e.displayedName("Example key 11")
        e.assignmentOptional().defaultValue([10,20,30])
        e.reconfigurable()
        e.commit()
        
        e = VECTOR_DOUBLE_ELEMENT(expected).key("exampleKey12")
        e.assignmentOptional().defaultValueFromString("1.1, -2.2, 3.3")
        e.reconfigurable()
        e.commit()
        
        e = INT32_ELEMENT(expected).key("exampleIntKey")
        e.assignmentOptional().defaultValueFromString("20")
        e.reconfigurable()
        e.commit()
        
        e = PATH_ELEMENT(expected).key("testPath")
        e.alias(5)
        e.displayedName("Filename")
        e.isOutputFile()
        e.options("file1, file2")
        e.assignmentOptional().defaultValue("karabo.log")
        e.reconfigurable()
        e.commit()

        e = PATH_ELEMENT(expected).key("testPath2")
        e.isInputFile()
        e.readOnly().alarmHigh("a").alarmLow("b").warnHigh("c").warnLow("d")
        e.commit()
        
        e = PATH_ELEMENT(expected).key("testPath3")
        e.isDirectory()
        e.assignmentMandatory()
        e.commit()