/* 
 * File:   HashFilter_Test.cc
 * Author: <krzysztof.wrona@xfel.eu>
 * 
 * Created on April 12, 2013, 11:56 AM
 */

#include "HashFilter_Test.hh"
#include <karabo/util/util.hpp>
#include <karabo/util/HashFilter.hh>
#include <karabo/io/TextSerializer.hh>
#include <karabo/io/h5/Format.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(HashFilter_Test);



namespace hashfilter {

    using namespace karabo::util;

    struct A {

        KARABO_CLASSINFO(A, "A", "1.0");
        KARABO_CONFIGURATION_BASE_CLASS;


        static void expectedParameters(karabo::util::Schema & expected) {

            STRING_ELEMENT(expected).key("a")
                    .description("a")
                    .displayedName("a")
                    .assignmentOptional().defaultValue("a value")
                    .tags("CY,CY,NC,JS,KW,NC")
                    .commit();

            STRING_ELEMENT(expected).key("exampleKey1")
                    .tags("BH,CY")
                    .displayedName("Example key 1")
                    .description("Example key 1 description")
                    .options("Radio,Air Condition,Navigation", ",")
                    .assignmentOptional().defaultValue("exampleValue1")
                    .reconfigurable()
                    .commit();

            INT32_ELEMENT(expected).key("exampleKey2").alias(10)
                    .tags("BH")
                    .displayedName("Example key 2")
                    .description("Example key 2 description")
                    .options("5, 25, 10")
                    .minInc(5)
                    .maxInc(25)
                    .unit(Units::METER)
                    .metricPrefix(Units::MILLI)
                    .assignmentOptional().defaultValue(2)
                    .init()
                    .commit();

            UINT32_ELEMENT(expected).key("exampleKey3").alias(5.5)
                    .tags("CY,JS")
                    .displayedName("Example key 3")
                    .description("Example key 3 description")
                    .allowedStates("AllOk.Started, AllOk.Stopped, AllOk.Run.On, NewState") //TODO check
                    .minExc(10)
                    .maxExc(20)
                    .assignmentOptional().defaultValue(3)
                    .reconfigurable()
                    .commit();

            FLOAT_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
                    .tags("KW")
                    .displayedName("Example key 4")
                    .description("Example key 4 description")
                    .options("1.11     -2.22 5.55")
                    .assignmentOptional().defaultValue(4.0)
                    .commit();

            INT64_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
                    .tags("KW")
                    .displayedName("Example key 5")
                    .description("Example key 5 description")
                    .assignmentOptional().defaultValue(5)
                    .commit();


        }


        A(const Hash & configuration) {
        }
    };

    struct GraphicsRenderer2 {

        KARABO_CLASSINFO(GraphicsRenderer2, "GraphicsRenderer2", "1.0");
        KARABO_CONFIGURATION_BASE_CLASS;


        static void expectedParameters(karabo::util::Schema & expected) {

            BOOL_ELEMENT(expected).key("antiAlias")
                    .tags("prop")
                    .displayedName("Use Anti-Aliasing")
                    .description("You may switch of for speed")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .advanced()
                    .commit();

            STRING_ELEMENT(expected).key("color")
                    .tags("KW")
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
                    .commit();

            FLOAT_ELEMENT(expected).key("shapes.circle.radius")
                    .description("The radius of the circle")
                    .displayedName("Radius")
                    .minExc(0)
                    .maxExc(100)
                    .unit(Units::METER)
                    .metricPrefix(Units::MILLI)
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();

            NODE_ELEMENT(expected).key("shapes.rectangle")
                    .tags("BH, KW , CY")
                    .displayedName("Rectangle")
                    .description("A rectangle")
                    .commit();

            FLOAT_ELEMENT(expected).key("shapes.rectangle.b")
                    .tags("JS")
                    .description("Rectangle side - b")
                    .displayedName("Side B")
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();

            FLOAT_ELEMENT(expected).key("shapes.rectangle.c")
                    .tags("KW")
                    .description("Rectangle side - c")
                    .displayedName("Side C")
                    .assignmentOptional().defaultValue(10)
                    .init()
                    .commit();

            NODE_ELEMENT(expected).key("shapes.triangle")
                    .displayedName("triangle")
                    .description("A triangle (Node element containing no other elements)")
                    .commit();

            NODE_ELEMENT(expected).key("letter")
                    .displayedName("Letter")
                    .description("Letter")
                    //.appendParametersOf<karabo::io::h5::Format>()
                    .appendParametersOf<A>()
                    //defaultValue(vector<string>(1,"A"))
                    .commit();

//            NODE_ELEMENT(expected).key("format")
//                    .displayedName("format")
//                    .description("hdf5 format")
//                    .appendParametersOf<karabo::io::h5::Format>()
//                    .commit();


        }


        GraphicsRenderer2(const karabo::util::Hash & input) {

        }
    };


}


using namespace hashfilter;
using namespace std;
using namespace karabo::io;


KARABO_REGISTER_FOR_CONFIGURATION(A);
KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer2);


HashFilter_Test::HashFilter_Test() {
}


HashFilter_Test::~HashFilter_Test() {
}


void HashFilter_Test::setUp() {
}


void HashFilter_Test::tearDown() {

}


void HashFilter_Test::testFilterByTag() {

    try {
        //Schema schema = A::getSchema("A");    
        //GraphicsRenderer2::expectedParameters(schema);
        Schema schema = Configurator<GraphicsRenderer2>::getSchema("GraphicsRenderer2");

        clog << "before validator" << endl;
        Validator validator;
        Hash config;
        validator.validate(schema, Hash(), config);

        const Hash& param = schema.getParameterHash1();


        clog << "\nparam : \n" << param << endl;



        //
        //    clog << archive << endl;
        //    
        clog << "\nconfig:\n" << config << endl;
        // 
        Hash result;
        HashFilter::byTag(result, schema, config, "KW;KW,BH",",;");

        clog << "\nresult: \n" << result << endl;

        CPPUNIT_ASSERT(1 == 1);
    } catch (karabo::util::Exception e) {
        cout << e << endl;
    }
}


void HashFilter_Test::testHdf5Filter() {

    using namespace karabo::io;

    Hash data("instrument.a", 10, "instrument.b", 2.4, "c", "Hello World");
    vector<unsigned short> vec(100,0);
    for(size_t i = 0; i< 100; ++i){
        vec[i] = i%20;
    }
    data.set("d", vec).setAttribute("dims", Dims(20,5).toVector());
    Hash config;
    h5::Format::discoverFromHash(data, config);
    h5::Format::Pointer dataFormat = h5::Format::createFormat(config);




    Hash i32el(
               "h5path", "experimental",
               "h5name", "test",
               "key", "experimental.test",
               "compressionLevel", 9
               );

    h5::Element::Pointer e1 = h5::Element::create("INT32", i32el);
    dataFormat->addElement(e1);


    try {


        Hash h5Config = dataFormat->getConfig();
        clog << "original\n" << h5Config << endl;
        Schema schema = h5::Format::getSchema("Format");
//        clog << "schema: \n" << schema.getParameterHash1() << endl;
        
        Hash result;
        HashFilter::byTag(result, schema, h5Config.get<Hash>("Format"), "persistent");

        clog << "permanent: \n" << result << endl;

    } catch (karabo::util::Exception e) {
        cout << e << endl;
    }

}

