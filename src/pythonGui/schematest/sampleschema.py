#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 4, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from karabo_decorators import *
from libkarathon import *


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("SampleSchema", "1.0")
class SampleSchema(object):


    def __init__(self, configuration):
        self.configuration = configuration


    @staticmethod
    def expectedParameters(expected):
        
        #e = DOUBLE_ELEMENT(expected)
        #e.key("targetSpeed").displayedName("Target Conveyor Speed").description("Configures the speed of the conveyor belt")
        #e.unit(VELOCITY)
        #e.unitSymbol("")
        #e.assignmentOptional().defaultValue(0.8)
        #e.minInc(0.0).maxInc(10.0)
        #e.assignmentMandatory()
        #e.reconfigurable().commit()

        #e = DOUBLE_ELEMENT(expected)
        #e.key("currentSpeed").displayedName("Current Conveyor Speed").description("Shows the current speed of the conveyor")
        #e.readOnly().commit()

        e = BOOL_ELEMENT(expected)
        e.key("reverseDirection").displayedName("Reverse Direction").description("Reverses the direction of the conveyor band")
        e.assignmentOptional().defaultValue(False)
        e.allowedStates("Ok.Stopped")
        e.reconfigurable().commit()
        
        e = PATH_ELEMENT(expected)
        e.key("filename").description("Name of the file to be read")
        e.displayedName("Filename")
        #e.assignmentMandatory()
        e.assignmentOptional()
        #e.isDirectory()
        e.isInputFile()
        #e.isOutputFile()
        e.commit()

        #e = VECTOR_INT32_ELEMENT(expected).key("exampleKey7")
        #e.displayedName("Example key 7")
        #e.allowedStates("Started, AllOk")
        #e.readOnly().initialValue([1,2,3]).alarmLow([-1,-1,-1]).alarmHigh([-2,2,-2]).warnLow([0,0,0]).warnHigh([10,20,30])
        #e.commit()
        
        #e = VECTOR_DOUBLE_ELEMENT(expected).key("exampleKey8")
        #e.readOnly().initialValue([1.1, 2.2, 3.3]).alarmLow([-1.1,-2.2,-3.3]).warnHigh([5.5, 7.7, 9.9]).commit()
        
        #e = VECTOR_INT32_ELEMENT(expected).key("exampleKey10")
        #e.displayedName("Example key 10")
        #e.minSize(2)
        #e.maxSize(7)
        #e.assignmentOptional().defaultValueFromString("10, 20, 30")
        #e.reconfigurable()
        #e.commit()
        
        #e = VECTOR_INT32_ELEMENT(expected).key("exampleKey11")
        #e.displayedName("Example key 11")
        #e.assignmentOptional().defaultValue([10,20,30])
        #e.reconfigurable()
        #e.commit()
        
        #e = VECTOR_DOUBLE_ELEMENT(expected).key("exampleKey12")
        #e.assignmentOptional().defaultValueFromString("1.1, -2.2, 3.3")
        #e.reconfigurable()
        #e.commit()
        
        e = VECTOR_STRING_ELEMENT(expected).key("exampleKey14")
        e.assignmentOptional().defaultValue(["Hallo", "World", "Test"])
        e.reconfigurable()
        e.commit()

        #e = VECTOR_INT32_ELEMENT(expected).key("vectorIntElement")
        #e.displayedName("V Element")
        #e.description("Vector Int Element for testing")
        #e.minSize(1)
        #e.maxSize(10)
        #e.assignmentOptional().defaultValueFromString("5,3,4")
        #e.reconfigurable()
        #e.commit()
      
        #e = VECTOR_STRING_ELEMENT(expected).key("vectorStringElement")
        #e.displayedName("Vector String Element")
        #e.description("Vector String Element for testing")
        #e.assignmentOptional().defaultValueFromString("string1,string2,string3")
        #e.reconfigurable()
        #e.commit()
        
        e = SLOT_ELEMENT(expected)
        e.key("start").displayedName("Start").description("Instructs device to go to started state")
        e.allowedStates("Ok.Stopped")
        e.commit()

        e = SLOT_ELEMENT(expected)
        e.key("stop").displayedName("Stop").description("Instructs device to go to stopped state")
        e.allowedStates("Ok.Started")
        e.commit()

        e = SLOT_ELEMENT(expected)
        e.key("reset").displayedName("Reset").description("Resets the device in case of an error")
        e.allowedStates("Error")
        e.commit()

        #e = CHOICE_ELEMENT(expected).key("shapes")
        #e.displayedName("Shape")
        #e.assignmentOptional().defaultValue("circle")
        #e.commit()

        #e = NODE_ELEMENT(expected).key("shapes.circle")
        #e.tags("shape")
        #e.displayedName("Circle")
        #e.description("A circle")
        #e.appendParametersOf(Circle)
        #e.commit()

        #e = NODE_ELEMENT(expected).key("shapes.rectangle")
        #e.tags("shape")
        #e.displayedName("Rectangle")
        #e.description("A rectangle")
        #e.commit()

        e = INT32_ELEMENT(expected)
        e.key("port").displayedName("Port").description("Input for port")
        e.assignmentOptional().defaultValue(2999)
        e.reconfigurable().commit()

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
        
        e = DOUBLE_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
        e.tags("software")
        e.displayedName("Example key 4")
        e.description("Example key 4 description")
        e.options("1.11,-2.22,5.55")
        e.minExc(-2.22).maxExc(5.55)
        e.assignmentInternal().noDefaultValue()
        e.commit()

        e = STRING_ELEMENT(expected).key("color")
        e.tags("prop")
        e.displayedName("Color")
        e.options("red,green,blue,orange,black")
        e.description("The default color for any shape")
        e.assignmentOptional().defaultValue("red")
        e.reconfigurable
        e.commit()

        e = STRING_ELEMENT(expected)
        e.key('firstWord').displayedName("First Word").description("Input for first word")
        e.assignmentOptional().defaultValue("Hello")
        e.reconfigurable().commit()

        e = STRING_ELEMENT(expected)
        e.key("secondWord").displayedName("Second Word").description("Input for first word")
        e.assignmentOptional().defaultValue("World")
        e.reconfigurable().commit()

        e = STRING_ELEMENT(expected)
        e.key("composedWord").displayedName("Composed word").description("The composed word")
        e.assignmentOptional().noDefaultValue()
        e.readOnly().commit()

        #e = STRING_ELEMENT(expected).key("testString")
        #e.tags("hardware, set")
        #e.displayedName("Example key 3")
        #e.description("Example key 3 description")
        #e.allowedStates("AllOk.Started, AllOk.Stopped, AllOk.Run.On, NewState") #TODO check
        #e.assignmentMandatory()
        #e.reconfigurable()
        #e.commit()

        e = DOUBLE_ELEMENT(expected).key("criticalDouble")
        e.displayedName("Critical double value")
        e.description("Critical double value description")
        e.readOnly().initialValue(1.11).alarmLow(-22.5).alarmHigh(22.5).warnLow(-10.0).warnHigh(10.0)
        e.commit()


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Shape", "1.0")
class Shape(object):
    
    def __init__(self, configuration):
        self.configuration = configuration
        
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

