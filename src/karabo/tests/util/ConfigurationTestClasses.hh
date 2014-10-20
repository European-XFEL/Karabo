/* 
 * Author: <burkhard.heisen>
 *
 * Created on February 5, 2013, 11:06 AM
 */

#ifndef KARABO_UTIL_TEST_EXPECTEDPARAMETERCLASSES_HH
#define	KARABO_UTIL_TEST_EXPECTEDPARAMETERCLASSES_HH

#include <karabo/util/SimpleElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/OverwriteElement.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/ListElement.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/xms/SlotElement.hh>
#include <karabo/util/ImageElement.hh>

#include <karabo/util/Configurator.hh>

#include <karabo/util/karaboDll.hh>

#include <boost/assign/std/vector.hpp> // for 'operator+=()'

namespace configurationTest {

    using namespace karabo::util;
    using namespace karabo::xms;


    struct Shape {
        KARABO_CLASSINFO(Shape, "Shape", "1.0");
        KARABO_CONFIGURATION_BASE_CLASS;

        static void expectedParameters(karabo::util::Schema & expected) {

            BOOL_ELEMENT(expected).key("shadowEnabled")
                    .description("Shadow enabled")
                    .displayedName("Shadow")
                    .assignmentOptional().defaultValue(false)
                    .init()
                    .commit();
        }

        Shape(const Hash & configuration) : m_configuration(configuration) {
        }

        virtual ~Shape() {
        }

        const Hash & getConfiguration() {
            return m_configuration;
        }

        virtual std::string draw() const = 0;

    private:
        Hash m_configuration;
    };

    //**********************************************
    //                Circle                       *
    //**********************************************


    struct Circle : public Shape {
        KARABO_CLASSINFO(Circle, "Circle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {

            FLOAT_ELEMENT(expected).key("radius").alias(1)
                    .description("The radius of the circle")
                    .displayedName("Radius")
                    .minExc(0)
                    .maxExc(100)
                    .unit(Unit::METER)
                    .metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();
        }

        Circle(const karabo::util::Hash & configuration) : Shape(configuration) {
        }

        virtual ~Circle() {
        }

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }
    };

    //**********************************************
    //            Editable Circle                  *
    //**********************************************


    struct EditableCircle : public Circle {
        KARABO_CLASSINFO(EditableCircle, "EditableCircle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {
            OVERWRITE_ELEMENT(expected).key("radius")
                    .setNowReconfigurable()
                    .commit();
        }

        EditableCircle(const karabo::util::Hash & configuration) : Circle(configuration) {
        }

        virtual ~EditableCircle() {
        }

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }

    };

    //**********************************************
    //                 Rectangle                   *
    //**********************************************


    struct Rectangle : public Shape {
        KARABO_CLASSINFO(Rectangle, "Rectangle", "1.0");

        static void expectedParameters(karabo::util::Schema & expected) {

            FLOAT_ELEMENT(expected).key("a").alias(1)
                    .description("Length of a")
                    .displayedName("A")
                    .minExc(0)
                    .maxExc(100)
                    .unit(Unit::METER)
                    .metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .adminAccess()
                    .init()
                    .commit();

            FLOAT_ELEMENT(expected).key("b").alias(1)
                    .description("Length of b")
                    .displayedName("B")
                    .minExc(0)
                    .maxExc(100)
                    .unit(Unit::METER)
                    .metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();
        }

        Rectangle(const karabo::util::Hash & configuration) : Shape(configuration) {
        }

        virtual ~Rectangle() {
        }

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }
    };


    struct GraphicsRenderer {
        KARABO_CLASSINFO(GraphicsRenderer, "GraphicsRenderer", "1.0")
        KARABO_CONFIGURATION_BASE_CLASS;

        static void expectedParameters(karabo::util::Schema & expected) {

            BOOL_ELEMENT(expected).key("antiAlias")
                    .tags("prop")
                    .displayedName("Use Anti-Aliasing")
                    .description("You may switch of for speed")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected).key("color")
                    .tags("prop")
                    .displayedName("Color")
                    .options("red,green,blue,orange,black")
                    .description("The default color for any shape")
                    .assignmentOptional().defaultValue("red")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("bold")
                    .tags("prop")
                    .displayedName("Bold")
                    .description("Toggles bold painting")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

            CHOICE_ELEMENT(expected).key("shapes")
                    .description("Some shapes")
                    .displayedName("Shapes")
                    .appendNodesOfConfigurationBase<Shape > ()
                    .assignmentOptional().defaultValue("Rectangle")
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected).key("version")
                    .displayedName("Version")
                    .description("Version information")
                    .readOnly()
                    .initialValue("1.4.7")
                    .commit();
        }

        GraphicsRenderer(const karabo::util::Hash & input) {
            //cout << input << endl;
            Shape::Pointer shape = Shape::createChoice("shapes", input);
            assert(input.get<string>("version") == "1.4.7");
            if (input.has("shapes.Circle")) assert(shape->draw() == "Circle");
        }

        virtual ~GraphicsRenderer() {
        }
    };


    struct GraphicsRenderer1 {
        KARABO_CLASSINFO(GraphicsRenderer1, "GraphicsRenderer1", "1.0");

        virtual ~GraphicsRenderer1() {
        }

        static void expectedParameters(karabo::util::Schema & expected) {

            BOOL_ELEMENT(expected).key("antiAlias")
                    .tags("prop")
                    .displayedName("Use Anti-Aliasing")
                    .description("You may switch of for speed")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected).key("color")
                    .tags("prop")
                    .displayedName("Color")
                    .description("The default color for any shape")
                    .assignmentOptional().defaultValue("red")
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("bold")
                    .tags("prop")
                    .displayedName("Bold")
                    .description("Toggles bold painting")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();

            CHOICE_ELEMENT(expected).key("shapes")
                    .assignmentOptional().defaultValue("circle")
                    .commit();

            NODE_ELEMENT(expected).key("shapes.circle")
                    .tags("shape")
                    .displayedName("Circle")
                    .description("A circle")
                    .appendParametersOf<Circle> ()
                    .commit();

            NODE_ELEMENT(expected).key("shapes.rectangle")
                    .tags("shape")
                    .displayedName("Rectangle")
                    .description("A rectangle")
                    .commit();

            FLOAT_ELEMENT(expected).key("shapes.rectangle.b")
                    .description("Rectangle side - b")
                    .displayedName("Side B")
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();

            FLOAT_ELEMENT(expected).key("shapes.rectangle.c")
                    .description("Rectangle side - c")
                    .displayedName("Side C")
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();

            NODE_ELEMENT(expected).key("triangle")
                    .displayedName("triangle")
                    .description("A triangle (Node element containing no other elements)")
                    .commit();
        }
    };


    struct TestStruct1 {
        KARABO_CLASSINFO(TestStruct1, "TestStruct1", "1.0");

        TestStruct1(const karabo::util::Hash& config) {
            
        }

        virtual ~TestStruct1() {
        }

        static void expectedParameters(karabo::util::Schema & expected) {

            STRING_ELEMENT(expected).key("exampleKey1")
                    .tags("hardware, poll")
                    .displayedName("Example key 1")
                    .description("Example key 1 description")
                    .options("Radio,Air Condition,Navigation", ",")
                    .assignmentOptional().defaultValue("Navigation")
                    .userAccess()
                    .reconfigurable()
                    .commit();

            INT32_ELEMENT(expected).key("exampleKey2").alias(10)
                    .tags("hardware, poll")
                    .displayedName("Example key 2")
                    .description("Example key 2 description")
                    .options("5, 25, 10")
                    .minInc(5)
                    .maxInc(25)
                    .unit(Unit::METER)
                    .metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .operatorAccess()
                    .init()
                    .commit();

            UINT32_ELEMENT(expected).key("exampleKey3").alias(5.5)
                    .tags("hardware, set")
                    .displayedName("Example key 3")
                    .description("Example key 3 description")
                    .allowedStates("AllOk.Started, AllOk.Stopped, AllOk.Run.On, NewState")
                    .minExc(10)
                    .maxExc(20)
                    .assignmentMandatory()
                    .expertAccess()
                    .reconfigurable()
                    .commit();

            FLOAT_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
                    .tags("software")
                    .displayedName("Example key 4")
                    .description("Example key 4 description")
                    .options("1.11     -2.22 5.55")
                    .adminAccess()
                    .assignmentInternal().noDefaultValue()
                    .commit();

            vector<int> vecIntAlias;
            {
                using namespace boost::assign;    // bring 'operator+=()' into scope
                vecIntAlias += 10, 20, 30;        // use boost::assign to initialize vector
            }
            INT64_ELEMENT(expected).key("exampleKey5").alias(vecIntAlias)
                    .tags("h/w; d.m.y", ";")
                    .displayedName("Example key 5")
                    .description("Example key 5 description")
                    .readOnly()
                    .initialValue(1442244)
                    .warnLow(-10).warnHigh(10)
                    .alarmLow(-20).alarmHigh(20)
                    .commit();

            UINT32_ELEMENT(expected).key("exampleKey6").alias("key6")
                    .displayedName("IP address")
                    .description("This is IP address presented in hex")
                    .hex()
                    .reconfigurable()
                    .operatorAccess()
                    .assignmentOptional().defaultValue(0x0100007f) // 127.0.0.1 == localhost
                    .commit();
            
            UINT16_ELEMENT(expected).key("exampleKey7").alias("key7")
                    .displayedName("Bit string")
                    .description("Example key 7 description")
                    .bin("0:isError,1:isMoving,2:isBusy,15:isOn")
                    .readOnly()
                    .initialValue(0)
                    .commit();
                    
            INT32_ELEMENT(expected).key("sampleKey")
                    .assignmentOptional().defaultValueFromString("10")
                    .reconfigurable()
                    .commit();

            INT32_ELEMENT(expected).key("sampleKey2")
                    .readOnly()
                    .commit();
        }
    };

    struct TestStruct2 : public TestStruct1 {
        KARABO_CLASSINFO(TestStruct2, "TestStruct2", "1.0");

        TestStruct2(const karabo::util::Hash& config) : TestStruct1(config) {

        }

        static void expectedParameters(Schema& schema) {

            OVERWRITE_ELEMENT(schema).key("exampleKey2")
                    .setNewAlias<int>(20)
                    .commit();

            OVERWRITE_ELEMENT(schema).key("exampleKey3")
                    .setNewAlias<int>(20)
                    .commit();


        }

    };


    struct OtherSchemaElements {
        KARABO_CLASSINFO(OtherSchemaElements, "OtherSchemaElements", "1.0");

        virtual ~OtherSchemaElements() {
        }

        static void expectedParameters(karabo::util::Schema & expected) {
            SLOT_ELEMENT(expected).key("slotTest")
                    .displayedName("Reset")
                    .description("Test slot element")
                    .allowedStates("Started, Stopped, Reset")
                    .commit();

            PATH_ELEMENT(expected)
                    .description("File name")
                    .key("filename")
                    .alias(5)
                    .displayedName("Filename")
                    .isOutputFile()
                    .options("file1, file2")
                    .assignmentOptional().defaultValue("karabo.log")
                    .reconfigurable()
                    .commit();

            PATH_ELEMENT(expected)
                    .key("testfile")
                    .isInputFile()
                    .readOnly().initialValue("initFile")
                    .alarmHigh("a").alarmLow("b")
                    .warnHigh("c").warnLow("d")
                    .archivePolicy(Schema::EVERY_10MIN)
                    .commit();

            vector<int> vecInit;
            {
                using namespace boost::assign;    // bring 'operator+=()' into scope
                vecInit += 10, 20, 30;
            }

            vector<int> vecWarnL(3, 50);
            vector<int> vecWarnH(3, 100);

            VECTOR_INT32_ELEMENT(expected)
                    .key("vecInt")
                    .readOnly()
                    .initialValue(vecInit)
                    .warnLow(vecWarnL)
                    .warnHigh(vecWarnH)
                    .archivePolicy(Schema::EVERY_EVENT)
                    .commit();

            vector<double> vecAlarmL(3, -5.5);
            vector<double> vecAlarmH(3, 7.7);
            VECTOR_DOUBLE_ELEMENT(expected)
                    .key("vecDouble")
                    .readOnly()
                    .alarmLow(vecAlarmL)
                    .alarmHigh(vecAlarmH)
                    .archivePolicy(Schema::NO_ARCHIVING)
                    .commit();

            VECTOR_INT32_ELEMENT(expected)
                    .key("vecIntReconfig")
                    .assignmentOptional().defaultValue(vecInit)
                    .reconfigurable()
                    .commit();

            VECTOR_INT32_ELEMENT(expected)
                    .key("vecIntReconfigStr")
                    .assignmentOptional().defaultValueFromString("11, 22, 33")
                    .reconfigurable()
                    .commit();
            
            VECTOR_DOUBLE_ELEMENT(expected)
                    .key("vecDoubleReconfigStr")
                    .assignmentOptional().defaultValueFromString("1.1, 2.2, 3.3")
                    .reconfigurable()
                    .commit();
            
            VECTOR_BOOL_ELEMENT(expected)
                    .key("vecBool")
                    .tags("h/w; d.m.y", ";")
                    .allowedStates("AllOk.Started, AllOk.Stopped")
                    .minSize(2)
                    .maxSize(7)
                    .assignmentMandatory()
                    .commit();
            
            IMAGE_ELEMENT(expected)
                    .key("image")
                    .commit();

        }
    };

    struct SchemaNodeElements {
        KARABO_CLASSINFO(SchemaNodeElements, "SchemaNodeElements", "1.0");

        SchemaNodeElements(const karabo::util::Hash& config) {
        }

        virtual ~SchemaNodeElements() {
        }

        static void expectedParameters(karabo::util::Schema & expected) {
            
            NODE_ELEMENT(expected).key("monitor")
                    .displayedName("Monitor")
                    .description("A Monitor (Node element containing count and other Node elements: stats)")
                    .commit();
            
            UINT32_ELEMENT(expected).key("monitor.count")
                    .displayedName("Count")
                    .description("Test count element")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(777)
                    .commit();

            NODE_ELEMENT(expected).key("monitor.stats")
                    .description("Complex status node empty for a while...")
                    .displayedName("Stats")
                    .commit();

        }
    };
    
    struct SchemaNodeInjected {
        KARABO_CLASSINFO(SchemaNodeInjected, "SchemaNodeInjected", "1.0");
        
        SchemaNodeInjected(const karabo::util::Hash& config) {
        }

        virtual ~SchemaNodeInjected() {
        }

        static void expectedParameters(karabo::util::Schema & expected) {

            NODE_ELEMENT(expected).key("monitor")
                    .displayedName("Monitor new")
                    .description("A Monitor new (Node element containing count and other Node elements: stats)")
                    .commit();
            
            NODE_ELEMENT(expected).key("monitor.stats")
                    .description("Complex status node having d1 parameter")
                    .displayedName("Stats")
                    .commit();

            FLOAT_ELEMENT(expected).key("monitor.stats.d1")
                    .description("D1 parameter in 'monitor.stats' node")
                    .displayedName("D1")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3.1415f)
                    .commit();
            
        }
    };
}
#endif