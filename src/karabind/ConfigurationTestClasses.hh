/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARABO_UTIL_TEST_EXPECTEDPARAMETERCLASSES_HH
#define KARABO_UTIL_TEST_EXPECTEDPARAMETERCLASSES_HH

#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <karabo/xms/ImageData.hh>
#include <karabo/xms/SlotElement.hh>

#include "karabo/data/schema/AlarmConditionElement.hh"
#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/schema/NDArrayElement.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/OverwriteElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/StateElement.hh"
#include "karabo/data/schema/TableElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/types/State.hh"
#include "karabo/data/types/karaboDll.hh"

namespace configurationTest {

    using namespace karabo::data;
    using namespace karabo::xms;

    struct Shape {
        KARABO_CLASSINFO(Shape, "Shape", "1.0");
        KARABO_CONFIGURATION_BASE_CLASS;

        static void expectedParameters(karabo::data::Schema& expected) {
            BOOL_ELEMENT(expected)
                  .key("shadowEnabled")
                  .description("Shadow enabled")
                  .displayedName("Shadow")
                  .assignmentOptional()
                  .defaultValue(false)
                  .init()
                  .commit();
        }

        Shape(const Hash& configuration) : m_configuration(configuration) {}

        virtual ~Shape() {}

        const Hash& getConfiguration() {
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

        static void expectedParameters(karabo::data::Schema& expected) {
            FLOAT_ELEMENT(expected)
                  .key("radius")
                  .alias(1)
                  .description("The radius of the circle")
                  .displayedName("Radius")
                  .minExc(0)
                  .maxExc(100)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(10)
                  .init()
                  .commit();

            STATE_ELEMENT(expected).key("state").commit();

            STATE_ELEMENT(expected).key("stateN").initialValue(State::NORMAL).commit();

            STATE_ELEMENT(expected).key("stateE").initialValue(State::ERROR).commit();

            ALARM_ELEMENT(expected).key("alarm").commit();

            ALARM_ELEMENT(expected).key("alarmW").initialValue(AlarmCondition::WARN).commit();

            ALARM_ELEMENT(expected).key("alarmA").initialValue(AlarmCondition::ALARM).commit();

            STRING_ELEMENT(expected).key("status").readOnly().commit();
        }

        Circle(const karabo::data::Hash& configuration) : Shape(configuration) {}

        virtual ~Circle() {}

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }
    };

    //**********************************************
    //            Editable Circle                  *
    //**********************************************

    struct EditableCircle : public Circle {
        KARABO_CLASSINFO(EditableCircle, "EditableCircle", "1.0");

        static void expectedParameters(karabo::data::Schema& expected) {
            OVERWRITE_ELEMENT(expected).key("radius").setNowReconfigurable().commit();

            OVERWRITE_ELEMENT(expected)
                  .key("state")
                  .setNewOptions(State::INIT, State::ERROR, State::NORMAL)
                  .setNewDefaultValue(State::INIT)
                  .commit();

            OVERWRITE_ELEMENT(expected).key("status").setNewOptions("a,b,c").setNewDefaultValue("a").commit();
        }

        EditableCircle(const karabo::data::Hash& configuration) : Circle(configuration) {}

        virtual ~EditableCircle() {}

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }
    };

    //**********************************************
    //                 Rectangle                   *
    //**********************************************

    struct Rectangle : public Shape {
        KARABO_CLASSINFO(Rectangle, "Rectangle", "1.0");

        static void expectedParameters(karabo::data::Schema& expected) {
            FLOAT_ELEMENT(expected)
                  .key("a")
                  .alias(1)
                  .description("Length of a")
                  .displayedName("A")
                  .minExc(0)
                  .maxExc(100)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(10)
                  .expertAccess()
                  .init()
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("b")
                  .alias(1)
                  .description("Length of b")
                  .displayedName("B")
                  .minExc(0)
                  .maxExc(100)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(10)
                  .init()
                  .commit();
        }

        Rectangle(const karabo::data::Hash& configuration) : Shape(configuration) {}

        virtual ~Rectangle() {}

        std::string draw() const {
            return this->getClassInfo().getClassId();
        }
    };

    struct GraphicsRenderer {
        KARABO_CLASSINFO(GraphicsRenderer, "GraphicsRenderer", "1.0")
        KARABO_CONFIGURATION_BASE_CLASS;

        static void expectedParameters(karabo::data::Schema& expected) {
            BOOL_ELEMENT(expected)
                  .key("antiAlias")
                  .tags("prop")
                  .displayedName("Use Anti-Aliasing")
                  .description("You may switch of for speed")
                  .assignmentOptional()
                  .defaultValue(true)
                  .init()
                  .expertAccess()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("color")
                  .tags("prop")
                  .displayedName("Color")
                  .options("red,green,blue,orange,black")
                  .description("The default color for any shape")
                  .assignmentOptional()
                  .defaultValue("red")
                  .reconfigurable()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("bold")
                  .tags("prop")
                  .displayedName("Bold")
                  .description("Toggles bold painting")
                  .assignmentOptional()
                  .defaultValue(false)
                  .reconfigurable()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("shapes")
                  .description("Some shapes")
                  .displayedName("Shapes")
                  .options("Circle, EditableCircle, Rectangle")
                  .assignmentOptional()
                  .defaultValue("Rectangle")
                  .expertAccess()
                  .commit();

            NODE_ELEMENT(expected)

                  .key("Circle")
                  .appendParametersOfConfigurableClass<Shape>("Circle")
                  .commit();

            NODE_ELEMENT(expected)

                  .key("EditableCircle")
                  .appendParametersOfConfigurableClass<Shape>("EditableCircle")
                  .commit();

            NODE_ELEMENT(expected)

                  .key("Rectangle")
                  .appendParametersOfConfigurableClass<Shape>("Rectangle")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("version")
                  .displayedName("Version")
                  .description("Version information")
                  .readOnly()
                  .initialValue("1.4.7")
                  .commit();
        }

        GraphicsRenderer(const karabo::data::Hash& input) {
            // cout << input << endl;
            std::string selected = input.get<std::string>("shapes");
            Shape::Pointer shape = Shape::create(selected, input.get<Hash>(selected));
            assert(input.get<std::string>("version") == "1.4.7");
            if (input.has("shapes.Circle")) assert(shape->draw() == "Circle");
        }

        virtual ~GraphicsRenderer() {}
    };

    struct GraphicsRenderer1 {
        KARABO_CLASSINFO(GraphicsRenderer1, "GraphicsRenderer1", "1.0");

        virtual ~GraphicsRenderer1() {}

        static void expectedParameters(karabo::data::Schema& expected) {
            BOOL_ELEMENT(expected)
                  .key("antiAlias")
                  .tags("prop")
                  .displayedName("Use Anti-Aliasing")
                  .description("You may switch of for speed")
                  .assignmentOptional()
                  .defaultValue(true)
                  .init()
                  .expertAccess()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("color")
                  .tags("prop")
                  .displayedName("Color")
                  .description("The default color for any shape")
                  .assignmentOptional()
                  .defaultValue("red")
                  .reconfigurable()
                  .allowedStates(State::OFF)
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("bold")
                  .tags("prop")
                  .displayedName("Bold")
                  .description("Toggles bold painting")
                  .assignmentOptional()
                  .defaultValue(false)
                  .reconfigurable()
                  .commit();

            STRING_ELEMENT(expected).key("shapes").assignmentOptional().defaultValue("circle").commit();

            NODE_ELEMENT(expected)
                  .key("circle")
                  .tags("shape")
                  .displayedName("Circle")
                  .description("A circle")
                  .appendParametersOf<Circle>()
                  .commit();

            NODE_ELEMENT(expected)
                  .key("rectangle")
                  .tags("shape")
                  .displayedName("Rectangle")
                  .description("A rectangle")
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("rectangle.b")
                  .description("Rectangle side - b")
                  .displayedName("Side B")
                  .tags("b")
                  .assignmentOptional()
                  .defaultValue(10)
                  .init()
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("rectangle.c")
                  .description("Rectangle side - c")
                  .displayedName("Side C")
                  .assignmentOptional()
                  .defaultValue(10)
                  .init()
                  .commit();

            NODE_ELEMENT(expected)
                  .key("triangle")
                  .displayedName("triangle")
                  .description("A triangle (Node element containing no other elements)")
                  .commit();
        }
    };

    struct TestStruct1 {
        KARABO_CLASSINFO(TestStruct1, "TestStruct1", "1.0");

        TestStruct1(const karabo::data::Hash& config) {}

        virtual ~TestStruct1() {}

        static void expectedParameters(karabo::data::Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("exampleKey1")
                  .tags("hardware, poll")
                  .displayedName("Example key 1")
                  .description("Example key 1 description")
                  .options("Radio,Air Condition,Navigation", ",")
                  .assignmentOptional()
                  .defaultValue("Navigation")
                  .operatorAccess()
                  .reconfigurable()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("exampleKey2")
                  .alias(10)
                  .tags("hardware, poll")
                  .displayedName("Example key 2")
                  .description("Example key 2 description")
                  .options("5, 25, 10")
                  .minInc(5)
                  .maxInc(25)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(10)
                  .operatorAccess()
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("exampleKey3")
                  .alias(5.5)
                  .tags("hardware, set")
                  .displayedName("Example key 3")
                  .description("Example key 3 description")
                  .allowedStates(State::STARTED, State::STOPPED, State::NORMAL)
                  .minExc(10)
                  .maxExc(20)
                  .assignmentMandatory()
                  .expertAccess()
                  .reconfigurable()
                  .commit();

            DOUBLE_ELEMENT(expected)
                  .key("exampleKey4")
                  .alias("exampleAlias4")
                  .tags("software")
                  .displayedName("Example key 4")
                  .description("Example key 4 description")
                  .options("1.11     -2.22 5.55")
                  .expertAccess()
                  .minExc(-2.22)
                  .maxExc(5.55)
                  .assignmentInternal()
                  .noDefaultValue()
                  .commit();

            INT64_ELEMENT(expected)
                  .key("exampleKey5")
                  .alias("exampleAlias5")
                  .tags("h/w; d.m.y", ";")
                  .displayedName("Example key 5")
                  .description("Example key 5 description")
                  .setSpecialDisplayType("Int64DisplayType")
                  .readOnly()
                  .initialValue(1442244)
                  .commit();

            INT64_ELEMENT(expected).key("exampleKey5b").readOnly().defaultValue(42).commit();

            DOUBLE_ELEMENT(expected)
                  .key("exampleKey6")
                  .alias(std::vector<int>({0x00123456, 0x0000aacc}))
                  .displayedName("Example key 6")
                  .description("Example key 6 description")
                  .readOnly()
                  .initialValue(1.11)
                  .commit();

            VECTOR_INT32_ELEMENT(expected)
                  .key("exampleKey7")
                  .displayedName("Example key 7")
                  .allowedStates(State::STARTED, State::NORMAL)
                  .readOnly()
                  .initialValue({1, 2, 3})
                  .commit();

            VECTOR_INT32_ELEMENT(expected)
                  .key("exampleKey7b")
                  .readOnly()
                  .initialValue(std::vector<int>({11, 22, 33}))
                  .commit();

            VECTOR_DOUBLE_ELEMENT(expected)
                  .key("exampleKey8")
                  .readOnly()
                  .initialValue({1.1, 2.2, 3.3})
                  .archivePolicy(Schema::NO_ARCHIVING)
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("exampleKey9")
                  .setSpecialDisplayType("TestDisplayType")
                  .readOnly()
                  .initialValue(std::vector<std::string>({"Hallo", "World"}))
                  .commit();

            VECTOR_INT32_ELEMENT(expected).key("vectInt").readOnly().commit();

            VECTOR_INT32_ELEMENT(expected)
                  .key("exampleKey10")
                  .displayedName("Example key 10")
                  .minSize(2)
                  .maxSize(7)
                  .assignmentOptional()
                  .defaultValueFromString("10, 20, 30")
                  .reconfigurable()
                  .commit();

            VECTOR_INT32_ELEMENT(expected)
                  .key("exampleKey11")
                  .displayedName("Example key 11")
                  .assignmentOptional()
                  .defaultValue({10, 20, 30})
                  .observerAccess()
                  .reconfigurable()
                  .commit();

            VECTOR_DOUBLE_ELEMENT(expected)
                  .key("exampleKey12")
                  .assignmentOptional()
                  .defaultValueFromString("1.1, -2.2, 3.3")
                  .reconfigurable()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("exampleKey14")
                  .assignmentOptional()
                  .defaultValue({"Hallo", "World", "Test"})
                  .reconfigurable()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("exampleKey15")
                  .assignmentOptional()
                  .defaultValueFromString("word1, word2, test")
                  .reconfigurable()
                  .commit();

            //             NDARRAY_ELEMENT(expected).key("exampleKey16")
            //             .dtype("UINT32")
            //             .shape({2, 5, 0})
            //             .commit();

            INT32_ELEMENT(expected)
                  .key("exampleIntKey")
                  .assignmentOptional()
                  .defaultValueFromString("20")
                  .reconfigurable()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("exampleBitsKey1")
                  .tags("hardware")
                  .displayedName("Example bits key 1")
                  .description("Example bits key 1 description")
                  .reconfigurable()
                  .bin()
                  .assignmentOptional()
                  .defaultValue(0xdeadbeef)
                  .commit();

            UINT64_ELEMENT(expected)
                  .key("exampleBitsKey2")
                  .tags("hardware")
                  .displayedName("Example bits key 2")
                  .description("Example bits key 2 description")
                  .reconfigurable()
                  .bin("10:In Error, 21:Busy, 35:HV On, 55:Crate On")
                  .assignmentOptional()
                  .defaultValue(0xdeadbeefdeadface)
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("exampleBitsKey3")
                  .tags("hardware")
                  .displayedName("Example bits key 3")
                  .description("Example bits key 3 description")
                  .reconfigurable()
                  .oct()
                  .assignmentOptional()
                  .defaultValue(0xbeefface)
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("exampleBitsKey4")
                  .tags("hardware")
                  .displayedName("Example bits key 4")
                  .description("Example bits key 4 description")
                  .reconfigurable()
                  .hex()
                  .assignmentOptional()
                  .defaultValue(0xbeefface)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("testPath")
                  .alias(5)
                  .displayedName("Filename")
                  .options("file1, file2, karabo.log")
                  .assignmentOptional()
                  .defaultValue("karabo.log")
                  .reconfigurable()
                  .commit();

            STRING_ELEMENT(expected).key("testPath2").readOnly().commit();

            STRING_ELEMENT(expected).key("testPath3").assignmentMandatory().commit();

            SLOT_ELEMENT(expected)
                  .key("slotTest")
                  .displayedName("Reset")
                  .description("Test slot element")
                  .allowedStates(State::STARTED, State::STOPPED, State::NORMAL)
                  .commit();

            Schema row = Schema();

            INT32_ELEMENT(row).key("int").readOnly().initialValue(0).commit();

            TABLE_ELEMENT(expected)
                  .key("tableI")
                  .setColumns(row)
                  .readOnly()
                  .initialValue(std::vector<Hash>({Hash("int", 2)}))
                  .commit();

            TABLE_ELEMENT(expected)
                  .key("tableD")
                  .setColumns(row)
                  .readOnly()
                  .defaultValue(std::vector<Hash>({Hash("int", 3)}))
                  .commit();

            NODE_ELEMENT(expected)
                  .key("myNode")
                  .displayedName("myNode")
                  .description("WidgetNode Test")
                  .setSpecialDisplayType("WidgetNode")
                  .commit();
        }
    };

    struct TestStruct2 : public TestStruct1 {
        KARABO_CLASSINFO(TestStruct2, "TestStruct2", "1.0");

        TestStruct2(const karabo::data::Hash& config) : TestStruct1(config) {}

        static void expectedParameters(Schema& schema) {
            OVERWRITE_ELEMENT(schema).key("exampleKey2").setNewAlias<int>(20).commit();

            OVERWRITE_ELEMENT(schema).key("exampleKey3").setNewAlias<int>(30).commit();
        }
    };

    struct OtherSchemaElements {
        KARABO_CLASSINFO(OtherSchemaElements, "OtherSchemaElements", "1.0");

        virtual ~OtherSchemaElements() {}

        static void expectedParameters(karabo::data::Schema& expected) {
            SLOT_ELEMENT(expected)
                  .key("slotTest")
                  .displayedName("Reset")
                  .description("Test slot element")
                  .allowedStates(State::STARTED, State::STOPPED, State::ERROR)
                  .commit();

            STRING_ELEMENT(expected)
                  .description("File name")
                  .key("filename")
                  .alias(5)
                  .displayedName("Filename")
                  .options("file1, file2, karabo.log")
                  .assignmentOptional()
                  .defaultValue("karabo.log")
                  .reconfigurable()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("testfile")
                  .readOnly()
                  .defaultValue("initFile") // (now) for readOnly it's the same as initialValue
                  .commit();

            using std::vector;

            vector<int> vecInit{10, 20, 30};
            vector<int> vecWarnL(3, 50);
            vector<int> vecWarnH(3, 100);

            VECTOR_INT32_ELEMENT(expected).key("vecInt").readOnly().initialValue(vecInit).commit();

            vector<double> vecAlarmL(3, -5.5);
            vector<double> vecAlarmH(3, 7.7);
            VECTOR_DOUBLE_ELEMENT(expected).key("vecDouble").readOnly().archivePolicy(Schema::NO_ARCHIVING).commit();

            VECTOR_INT32_ELEMENT(expected)
                  .key("vecIntReconfig")
                  .assignmentOptional()
                  .defaultValue(vecInit)
                  .reconfigurable()
                  .commit();

            VECTOR_INT32_ELEMENT(expected)
                  .key("vecIntReconfigStr")
                  .assignmentOptional()
                  .defaultValueFromString("11, 22, 33")
                  .reconfigurable()
                  .commit();

            VECTOR_DOUBLE_ELEMENT(expected)
                  .key("vecDoubleReconfigStr")
                  .assignmentOptional()
                  .defaultValueFromString("1.1, 2.2, 3.3")
                  .reconfigurable()
                  .commit();

            VECTOR_BOOL_ELEMENT(expected)
                  .key("vecBool")
                  .tags("h/w; d.m.y", ";")
                  .allowedStates(State::STARTED, State::STOPPED)
                  .minSize(2)
                  .maxSize(7)
                  .assignmentMandatory()
                  .commit();

            const std::vector<unsigned long long> shape32({3, 2});
            NDARRAY_ELEMENT(expected).key("arrBool").dtype(karabo::data::Types::BOOL).shape(shape32).commit();

            NDARRAY_ELEMENT(expected).key("arrInt8").dtype(karabo::data::Types::INT8).shape(shape32).commit();

            NDARRAY_ELEMENT(expected).key("arrUInt16").dtype(karabo::data::Types::UINT16).shape(shape32).commit();

            NDARRAY_ELEMENT(expected).key("arrFloat").dtype(karabo::data::Types::FLOAT).shape(shape32).commit();

            // Keep shape as string to also test that
            NDARRAY_ELEMENT(expected).key("arrDouble").dtype(karabo::data::Types::DOUBLE).shape("3,2,-1").commit();

            NDARRAY_ELEMENT(expected)
                  .key("arrUndefined")
                  // leave type undefined
                  .shape("0,3,0")
                  .commit();

            IMAGEDATA_ELEMENT(expected).key("image").commit();

            Schema rowSchema;

            INT32_ELEMENT(rowSchema)
                  .key("a")
                  .minInc(2)
                  .maxExc(10)
                  .displayedName("A")
                  .assignmentOptional()
                  .defaultValue(2)
                  .commit();

            STRING_ELEMENT(rowSchema).key("b").assignmentOptional().noDefaultValue().commit();

            TABLE_ELEMENT(expected)
                  .key("testTable")
                  .setColumns(rowSchema)
                  .assignmentOptional()
                  .defaultValue(std::vector<Hash>(2, Hash("a", 3, "b", "foo")))
                  .commit();

            TABLE_ELEMENT(expected)
                  .key("testTableEmptyDefault")
                  .setColumns(rowSchema)
                  .assignmentOptional()
                  .defaultValue(std::vector<Hash>())
                  .commit();
        }
    };

    struct SchemaNodeElements {
        KARABO_CLASSINFO(SchemaNodeElements, "SchemaNodeElements", "1.0");

        SchemaNodeElements(const karabo::data::Hash& config) {}

        virtual ~SchemaNodeElements() {}

        static void expectedParameters(karabo::data::Schema& expected) {
            NODE_ELEMENT(expected)
                  .key("monitor")
                  .displayedName("Monitor")
                  .description("A Monitor (Node element containing count and other Node elements: stats)")
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("monitor.count")
                  .displayedName("Count")
                  .description("Test count element")
                  .reconfigurable()
                  .assignmentOptional()
                  .defaultValue(777)
                  .commit();

            NODE_ELEMENT(expected)
                  .key("monitor.stats")
                  .description("Complex status node empty for a while...")
                  .displayedName("Stats")
                  .commit();
        }
    };

    struct SchemaNodeInjected {
        KARABO_CLASSINFO(SchemaNodeInjected, "SchemaNodeInjected", "1.0");

        SchemaNodeInjected(const karabo::data::Hash& config) {}

        virtual ~SchemaNodeInjected() {}

        static void expectedParameters(karabo::data::Schema& expected) {
            NODE_ELEMENT(expected)
                  .key("monitor")
                  .displayedName("Monitor new")
                  .description("A Monitor new (Node element containing count and other Node elements: stats)")
                  .commit();

            NODE_ELEMENT(expected)
                  .key("monitor.stats")
                  .description("Complex status node having d1 parameter")
                  .displayedName("Stats")
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("monitor.stats.d1")
                  .description("D1 parameter in 'monitor.stats' node")
                  .displayedName("D1")
                  .reconfigurable()
                  .assignmentOptional()
                  .defaultValue(3.1415f)
                  .commit();
        }
    };


    struct SomeClass {
        KARABO_CLASSINFO(SomeClass, "SomeClassId", "1.0")
        KARABO_CONFIGURATION_BASE_CLASS;

        SomeClass(const karabo::data::Hash& config) {}

        virtual ~SomeClass() {}

        static void expectedParameters(karabo::data::Schema& expected) {
            INT32_ELEMENT(expected)
                  .key("x")
                  .alias(10)
                  .tags("IK,BH")
                  .displayedName("Xkey")
                  .description("Example of X key description")
                  .options("5, 25, 10")
                  .minInc(5)
                  .maxInc(25)
                  .unit(Unit::AMPERE)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(5)
                  .init()
                  .expertAccess()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("y")
                  .alias("bla")
                  .tags("CY")
                  .displayedName("Ykey")
                  .description("Example of Y key description")
                  .options("5, 25, 10")
                  .minExc(0)
                  .maxExc(29)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::CENTI)
                  .assignmentOptional()
                  .defaultValue(10)
                  .init()
                  .commit();

            DOUBLE_ELEMENT(expected).key("a").readOnly().initialValue(1.11).commit();

            VECTOR_INT32_ELEMENT(expected)
                  .key("somelist")
                  .displayedName("Ykey")
                  .description("Example of Y key description")
                  .reconfigurable()
                  .assignmentOptional()
                  .defaultValue({})
                  .commit();

            IMAGEDATA_ELEMENT(expected)
                  .key("myImageElement")
                  .displayedName("myImage")
                  .description("Image Element")
                  .setDimensions("110, 210")
                  .operatorAccess()
                  .commit();
        }
    };
} // namespace configurationTest
#endif
