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
/*
 * File:   HashFilter_Test.cc
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on April 12, 2013, 11:56 AM
 */

#include "HashFilter_Test.hh"

#include <karabo/io.hpp>
#include <karabo/util.hpp>
#include <karabo/util/State.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(HashFilter_Test);


namespace hashfilter {

    using namespace karabo::util;


    struct Base {
        KARABO_CLASSINFO(Base, "Base", "1.0");
        KARABO_CONFIGURATION_BASE_CLASS;


        Base(const Hash& configuration) {}


        virtual ~Base() {}
    };


    struct P1 : public Base {
        KARABO_CLASSINFO(P1, "P1", "1.0");


        static void expectedParameters(karabo::util::Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("a")
                  .description("a")
                  .displayedName("a")
                  .assignmentOptional()
                  .defaultValue("a value")
                  .tags("CY,CY,NC,JS,KW,NC")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("b")
                  .tags("BH,CY")
                  .displayedName("Example key 1")
                  .description("Example key 1 description")
                  .options("Radio,Air Condition,Navigation", ",")
                  .assignmentOptional()
                  .defaultValue("exampleValue1")
                  .reconfigurable()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("c")
                  .alias(10)
                  .tags("BH")
                  .displayedName("Example key 2")
                  .description("Example key 2 description")
                  .options("5, 25, 10")
                  .minInc(5)
                  .maxInc(25)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(2)
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("d")
                  .alias(5.5)
                  .tags("CY,JS")
                  .displayedName("Example key 3")
                  .description("Example key 3 description")
                  .allowedStates(State::STARTED, State::STOPPED, State::ACTIVE) // TODO check
                  .minExc(10)
                  .maxExc(20)
                  .assignmentOptional()
                  .defaultValue(3)
                  .reconfigurable()
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("e")
                  .alias("exampleAlias4")
                  .tags("DB,NC,CY")
                  .displayedName("Example key 4")
                  .description("Example key 4 description")
                  .options("1.11     -2.22 5.55")
                  .assignmentOptional()
                  .defaultValue(4.0)
                  .commit();

            INT64_ELEMENT(expected)
                  .key("f")
                  .alias("exampleAlias5")
                  .tags("LM,DB")
                  .displayedName("Example key 5")
                  .description("Example key 5 description")
                  .assignmentOptional()
                  .defaultValue(5)
                  .commit();
        }


        P1(const Hash& configuration) : Base(configuration) {}


        virtual ~P1() {}
    };


    struct P2 : public Base {
        KARABO_CLASSINFO(P2, "P2", "1.0");


        static void expectedParameters(karabo::util::Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("x")
                  .description("x")
                  .displayedName("x")
                  .assignmentOptional()
                  .defaultValue("a value")
                  .tags("LM,BH")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("y")
                  .tags("CY")
                  .displayedName("Example key 1")
                  .description("Example key 1 description")
                  .options("Radio,Air Condition,Navigation", ",")
                  .assignmentOptional()
                  .defaultValue("exampleValue1")
                  .reconfigurable()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("z")
                  .alias(10)
                  .tags("CY,LM,KW")
                  .displayedName("Example key 2")
                  .description("Example key 2 description")
                  .options("5, 25, 10")
                  .minInc(5)
                  .maxInc(25)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(2)
                  .init()
                  .commit();
        }


        P2(const Hash& configuration) : Base(configuration) {}


        virtual ~P2() {}
    };


    struct P3 : public Base {
        KARABO_CLASSINFO(P3, "P3", "1.0");


        static void expectedParameters(karabo::util::Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("k")
                  .description("k")
                  .displayedName("k")
                  .assignmentOptional()
                  .defaultValue("k value")
                  .tags("LM")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("l")
                  .tags("CY")
                  .displayedName("l")
                  .description("l")
                  .options("Radio,Air Condition,Navigation", ",")
                  .assignmentOptional()
                  .defaultValue("exampleValue1")
                  .reconfigurable()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("m")
                  .alias(10)
                  .tags("CY,DB,JE,BP,MK,PG,BF")
                  .displayedName("Example key 2")
                  .description("Example key 2 description")
                  .options("5, 25, 10")
                  .minInc(5)
                  .maxInc(25)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(2)
                  .init()
                  .commit();
        }


        P3(const Hash& configuration) : Base(configuration) {}


        virtual ~P3() {}
    };


    struct GraphicsRenderer2 {
        KARABO_CLASSINFO(GraphicsRenderer2, "GraphicsRenderer2", "1.0");
        KARABO_CONFIGURATION_BASE_CLASS;


        static void expectedParameters(karabo::util::Schema& expected) {
            BOOL_ELEMENT(expected)
                  .key("antiAlias")
                  .tags("NC")
                  .displayedName("Use Anti-Aliasing")
                  .description("You may switch of for speed")
                  .assignmentOptional()
                  .defaultValue(true)
                  .init()
                  .expertAccess()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("color")
                  .tags("KW")
                  .displayedName("Color")
                  .description("The default color for any shape")
                  .assignmentOptional()
                  .defaultValue("red")
                  .reconfigurable()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("bold")
                  .tags("LM")
                  .displayedName("Bold")
                  .description("Toggles bold painting")
                  .assignmentOptional()
                  .defaultValue(false)
                  .reconfigurable()
                  .commit();

            CHOICE_ELEMENT(expected).key("shapes").tags("DB").assignmentOptional().defaultValue("rectangle").commit();

            NODE_ELEMENT(expected)
                  .key("shapes.circle")
                  .tags("JS")
                  .displayedName("Circle")
                  .description("A circle")
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("shapes.circle.radius")
                  .description("The radius of the circle")
                  .displayedName("Radius")
                  .tags("NC,KW")
                  .minExc(0)
                  .maxExc(100)
                  .unit(Unit::METER)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(10)
                  .init()
                  .commit();

            NODE_ELEMENT(expected)
                  .key("shapes.rectangle")
                  .tags("BH, KW , CY")
                  .displayedName("Rectangle")
                  .description("A rectangle")
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("shapes.rectangle.b")
                  .tags("JS")
                  .description("Rectangle side - b")
                  .displayedName("Side B")
                  .assignmentOptional()
                  .defaultValue(10)
                  .init()
                  .commit();

            FLOAT_ELEMENT(expected)
                  .key("shapes.rectangle.c")
                  .tags("LM,JS")
                  .description("Rectangle side - c")
                  .displayedName("Side C")
                  .assignmentOptional()
                  .defaultValue(10)
                  .init()
                  .commit();

            NODE_ELEMENT(expected)
                  .key("shapes.triangle")
                  .displayedName("triangle")
                  .description("A triangle (Node element containing no other elements)")
                  .commit();

            NODE_ELEMENT(expected)
                  .key("letter")
                  .displayedName("Letter")
                  .description("Letter")
                  .appendParametersOf<P1>()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("number")
                  .displayedName("Number")
                  .description("Integer number")
                  .tags("LM")
                  .readOnly()
                  .initialValue(12)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("state")
                  .displayedName("State")
                  .description("Status of application")
                  .tags("LM")
                  .readOnly()
                  .initialValue("Initialized")
                  .commit();

            Schema data;

            INT32_ELEMENT(data)
                  .key("number")
                  .displayedName("Number")
                  .description("Integer number")
                  .tags("LM") // The tags inside row schema are ignored: TableElement is LEAF
                  .readOnly()
                  .initialValue(12)
                  .commit();

            STRING_ELEMENT(data)
                  .key("state")
                  .displayedName("State")
                  .description("Status of application")
                  .readOnly()
                  .initialValue("Initialized")
                  .commit();

            BOOL_ELEMENT(data)
                  .key("bold")
                  .displayedName("Bold")
                  .description("Toggles bold painting")
                  .readOnly()
                  .initialValue(false)
                  .commit();

            TABLE_ELEMENT(expected)
                  .key("table")
                  .tags("LM") // The tags for the "whole" TableElement CAN work like for other parameters
                  .setColumns(data)
                  .assignmentOptional()
                  .defaultValue(std::vector<karabo::util::Hash>())
                  .commit();
        }


        GraphicsRenderer2(const karabo::util::Hash& input) {}


        virtual ~GraphicsRenderer2() {}
    };


} // namespace hashfilter


using namespace hashfilter;
using namespace std;
using namespace karabo::io;


KARABO_REGISTER_FOR_CONFIGURATION(Base, P1);
KARABO_REGISTER_FOR_CONFIGURATION(Base, P2);
KARABO_REGISTER_FOR_CONFIGURATION(Base, P3);
KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer2);


HashFilter_Test::HashFilter_Test() {}


HashFilter_Test::~HashFilter_Test() {}


void HashFilter_Test::setUp() {}


void HashFilter_Test::tearDown() {}


void HashFilter_Test::testFilterByTag() {
    try {
        Schema schema = Configurator<GraphicsRenderer2>::getSchema("GraphicsRenderer2");

        Validator validator;
        Hash config;
        validator.validate(schema, Hash(), config);

        Hash result;

        HashFilter::byTag(schema, config, result, "KW;KW,BH", ",;");

        CPPUNIT_ASSERT(result.has("antiAlias") == false);
        CPPUNIT_ASSERT(result.has("color") == true);
        CPPUNIT_ASSERT(result.has("bold") == false);
        CPPUNIT_ASSERT(result.has("shapes") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == true);
        CPPUNIT_ASSERT(result.has("letter") == true);
        CPPUNIT_ASSERT(result.has("letter.a") == true);
        CPPUNIT_ASSERT(result.has("letter.b") == true);
        CPPUNIT_ASSERT(result.has("letter.c") == true);
        CPPUNIT_ASSERT(result.has("letter.d") == false);
        CPPUNIT_ASSERT(result.has("letter.e") == false);
        CPPUNIT_ASSERT(result.has("letter.f") == false);
        CPPUNIT_ASSERT(result.has("table") == false);


        result.clear();
        HashFilter::byTag(schema, config, result, "JS", ",;");

        //        KARABO_LOG_FRAMEWORK_DEBUG << "JS\n" << result;


        CPPUNIT_ASSERT(result.has("antiAlias") == false);
        CPPUNIT_ASSERT(result.has("color") == false);
        CPPUNIT_ASSERT(result.has("bold") == false);
        CPPUNIT_ASSERT(result.has("shapes") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == true);
        CPPUNIT_ASSERT(result.has("letter") == true);
        CPPUNIT_ASSERT(result.has("letter.a") == true);
        CPPUNIT_ASSERT(result.has("letter.b") == false);
        CPPUNIT_ASSERT(result.has("letter.c") == false);
        CPPUNIT_ASSERT(result.has("letter.d") == true);
        CPPUNIT_ASSERT(result.has("letter.e") == false);
        CPPUNIT_ASSERT(result.has("letter.f") == false);
        CPPUNIT_ASSERT(result.has("table") == false);


        result.clear();
        HashFilter::byTag(schema, config, result, "NC,LM", ",;");

        //        KARABO_LOG_FRAMEWORK_DEBUG << "NC,LM\n" << result;


        CPPUNIT_ASSERT(result.has("antiAlias") == true);
        CPPUNIT_ASSERT(result.has("color") == false);
        CPPUNIT_ASSERT(result.has("bold") == true);
        CPPUNIT_ASSERT(result.has("shapes") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == true);
        CPPUNIT_ASSERT(result.has("letter") == true);
        CPPUNIT_ASSERT(result.has("letter.a") == true);
        CPPUNIT_ASSERT(result.has("letter.b") == false);
        CPPUNIT_ASSERT(result.has("letter.c") == false);
        CPPUNIT_ASSERT(result.has("letter.d") == false);
        CPPUNIT_ASSERT(result.has("letter.e") == true);
        CPPUNIT_ASSERT(result.has("letter.f") == true);
        CPPUNIT_ASSERT(result.has("table") == true);


        result.clear();
        HashFilter::byTag(schema, config, result, "CY", ",;");

        //        KARABO_LOG_FRAMEWORK_DEBUG << "CY\n" << result;


        CPPUNIT_ASSERT(result.has("antiAlias") == false);
        CPPUNIT_ASSERT(result.has("color") == false);
        CPPUNIT_ASSERT(result.has("bold") == false);
        CPPUNIT_ASSERT(result.has("shapes") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == true);
        CPPUNIT_ASSERT(result.has("letter") == true);
        CPPUNIT_ASSERT(result.has("letter.a") == true);
        CPPUNIT_ASSERT(result.has("letter.b") == true);
        CPPUNIT_ASSERT(result.has("letter.c") == false);
        CPPUNIT_ASSERT(result.has("letter.d") == true);
        CPPUNIT_ASSERT(result.has("letter.e") == true);
        CPPUNIT_ASSERT(result.has("letter.f") == false);
        CPPUNIT_ASSERT(result.has("table") == false);


        result.clear();
        HashFilter::byTag(schema, config, result, "BF", ",;");

        //        KARABO_LOG_FRAMEWORK_DEBUG << "BF\n" << result;


        CPPUNIT_ASSERT(result.has("antiAlias") == false);
        CPPUNIT_ASSERT(result.has("color") == false);
        CPPUNIT_ASSERT(result.has("bold") == false);
        CPPUNIT_ASSERT(result.has("shapes") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == false);
        CPPUNIT_ASSERT(result.has("letter") == false);
        CPPUNIT_ASSERT(result.has("letter.a") == false);
        CPPUNIT_ASSERT(result.has("letter.b") == false);
        CPPUNIT_ASSERT(result.has("letter.c") == false);
        CPPUNIT_ASSERT(result.has("letter.d") == false);
        CPPUNIT_ASSERT(result.has("letter.e") == false);
        CPPUNIT_ASSERT(result.has("letter.f") == false);
        CPPUNIT_ASSERT(result.has("table") == false);


        result.clear();
        HashFilter::byTag(schema, config, result, "WP76", ",;");

        //        KARABO_LOG_FRAMEWORK_DEBUG << "WP76\n" << result;


        CPPUNIT_ASSERT(result.has("antiAlias") == false);
        CPPUNIT_ASSERT(result.has("color") == false);
        CPPUNIT_ASSERT(result.has("bold") == false);
        CPPUNIT_ASSERT(result.has("shapes") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == false);
        CPPUNIT_ASSERT(result.has("letter") == false);
        CPPUNIT_ASSERT(result.has("letter.a") == false);
        CPPUNIT_ASSERT(result.has("letter.b") == false);
        CPPUNIT_ASSERT(result.has("letter.c") == false);
        CPPUNIT_ASSERT(result.has("letter.d") == false);
        CPPUNIT_ASSERT(result.has("letter.e") == false);
        CPPUNIT_ASSERT(result.has("letter.f") == false);
        CPPUNIT_ASSERT(result.has("table") == false);


    } catch (const karabo::util::Exception& e) {
        KARABO_LOG_FRAMEWORK_DEBUG << e;
    }
}


void HashFilter_Test::testFilterByAccessMode() {
    try {
        Schema schema = Configurator<GraphicsRenderer2>::getSchema("GraphicsRenderer2");

        Validator validator;
        Hash config;
        validator.validate(schema, Hash(), config);

        Hash result;
        HashFilter::byAccessMode(schema, config, result, karabo::util::INIT);

        KARABO_LOG_FRAMEWORK_DEBUG << "\nINIT ...\n" << result;

        CPPUNIT_ASSERT(result.has("antiAlias") == true);
        CPPUNIT_ASSERT(result.has("color") == false);
        CPPUNIT_ASSERT(result.has("bold") == false);
        CPPUNIT_ASSERT(result.has("shapes") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == true);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == true);
        CPPUNIT_ASSERT(result.has("letter") == true);
        CPPUNIT_ASSERT(result.has("letter.a") == true);
        CPPUNIT_ASSERT(result.has("letter.b") == false);
        CPPUNIT_ASSERT(result.has("letter.c") == true);
        CPPUNIT_ASSERT(result.has("letter.d") == false);
        CPPUNIT_ASSERT(result.has("letter.e") == true);
        CPPUNIT_ASSERT(result.has("letter.f") == true);
        CPPUNIT_ASSERT(result.has("number") == false);
        CPPUNIT_ASSERT(result.has("state") == false);
        CPPUNIT_ASSERT(result.has("table") == true);

        result.clear();
        HashFilter::byAccessMode(schema, config, result, karabo::util::READ);

        KARABO_LOG_FRAMEWORK_DEBUG << "\nREAD ...\n" << result;


        CPPUNIT_ASSERT(result.has("antiAlias") == false);
        CPPUNIT_ASSERT(result.has("color") == false);
        CPPUNIT_ASSERT(result.has("bold") == false);
        CPPUNIT_ASSERT(result.has("shapes") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == false);
        CPPUNIT_ASSERT(result.has("letter") == false);
        CPPUNIT_ASSERT(result.has("letter.a") == false);
        CPPUNIT_ASSERT(result.has("letter.b") == false);
        CPPUNIT_ASSERT(result.has("letter.c") == false);
        CPPUNIT_ASSERT(result.has("letter.d") == false);
        CPPUNIT_ASSERT(result.has("letter.e") == false);
        CPPUNIT_ASSERT(result.has("letter.f") == false);
        CPPUNIT_ASSERT(result.has("number") == true);
        CPPUNIT_ASSERT(result.has("state") == true);
        CPPUNIT_ASSERT(result.has("table") == false);


        result.clear();
        HashFilter::byAccessMode(schema, config, result, karabo::util::WRITE);

        KARABO_LOG_FRAMEWORK_DEBUG << "\nWRITE ...\n" << result;


        CPPUNIT_ASSERT(result.has("antiAlias") == false);
        CPPUNIT_ASSERT(result.has("color") == true);
        CPPUNIT_ASSERT(result.has("bold") == true);
        CPPUNIT_ASSERT(result.has("shapes") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.b") == false);
        CPPUNIT_ASSERT(result.has("shapes.rectangle.c") == false);
        CPPUNIT_ASSERT(result.has("letter") == true);
        CPPUNIT_ASSERT(result.has("letter.a") == false);
        CPPUNIT_ASSERT(result.has("letter.b") == true);
        CPPUNIT_ASSERT(result.has("letter.c") == false);
        CPPUNIT_ASSERT(result.has("letter.d") == true);
        CPPUNIT_ASSERT(result.has("letter.e") == false);
        CPPUNIT_ASSERT(result.has("letter.f") == false);
        CPPUNIT_ASSERT(result.has("number") == false);
        CPPUNIT_ASSERT(result.has("state") == false);
        CPPUNIT_ASSERT(result.has("table") == false);

    } catch (const karabo::util::Exception& e) {
        KARABO_LOG_FRAMEWORK_DEBUG << e;
    }
}


#ifdef HASHFILTER_HDF5TEST


void HashFilter_Test::testHdf5Filter() {
    using namespace karabo::io;

    Hash data("instrument.a", 10, "instrument.b", 2.4, "c", "Hello World");
    vector<unsigned short> vec(100, 0);
    for (size_t i = 0; i < 100; ++i) {
        vec[i] = i % 20;
    }
    data.set("d", vec).setAttribute("dims", Dims(20, 5).toVector());
    Hash config;
    h5::Format::discoverFromHash(data, config);
    h5::Format::Pointer dataFormat = h5::Format::createFormat(config);


    Hash i32el("h5path", "experimental", "h5name", "test", "key", "experimental.test", "compressionLevel", 9);

    h5::Element::Pointer e1 = h5::Element::create("INT32", i32el);
    dataFormat->addElement(e1);


    try {
        Hash h5Config = dataFormat->getConfig();
        KARABO_LOG_FRAMEWORK_DEBUG << "original\n" << h5Config;
        Schema schema = h5::Format::getSchema("Format");
        //        KARABO_LOG_FRAMEWORK_DEBUG << "schema: \n" << schema.getParameterHash1();

        Hash result;
        HashFilter::byTag(schema, h5Config.get<Hash>("Format"), result, "persistent");

        KARABO_LOG_FRAMEWORK_DEBUG << "permanent: \n" << result;

    } catch (const karabo::util::Exception& e) {
        KARABO_LOG_FRAMEWORK_DEBUG << e;
    }
}
#endif
