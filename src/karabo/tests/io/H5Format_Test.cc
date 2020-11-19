/*
 * File:   H5Format_Test.cc
 * Author: wrona
 *
 * Created on Feb 13, 2013, 11:21:54 AM
 */

#include "H5Format_Test.hh"

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include <karabo/util/Configurator.hh>
#include <karabo/io/TextSerializer.hh>

#include <karabo/io/h5/Format.hh>
#include <karabo/io/h5/Element.hh>



#include "karabo/util/TimeProfiler.hh"
#include <karabo/util/ArrayTools.hh>

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::io::h5;
using namespace boost;
using std::vector;

CPPUNIT_TEST_SUITE_REGISTRATION(H5Format_Test);


H5Format_Test::H5Format_Test() {

}


H5Format_Test::~H5Format_Test() {
}


void H5Format_Test::setUp() {
}


void H5Format_Test::tearDown() {
}


void H5Format_Test::testEmptyFormat() {
    Format::Pointer format = Format::createEmptyFormat();
    const Hash& config = format->getConfig();
    //    clog << endl << "Empty format config\n" << format->getConfig() << endl;
    CPPUNIT_ASSERT(config.has("Format") == true);
    CPPUNIT_ASSERT(config.is<Hash > ("Format") == true);

    CPPUNIT_ASSERT(config.get<vector<Hash> >("Format.elements").size() == 0);
    //clog << "Empty format config\n" << format->getConfig() << endl;    
}


void H5Format_Test::testManualFormat() {


    Format::Pointer format = Format::createEmptyFormat();
    Hash c1(
            "h5path", "experimental",
            "h5name", "test23",
            "key", "instrument.test",
            "compressionLevel", 9
            );

    h5::Element::Pointer e1 = h5::Element::create("UINT32", c1);
    format->addElement(e1);

    const Hash config1 = format->getConfig();
    KARABO_LOG_FRAMEWORK_TRACE << "config1:\n" << config1;

    const vector<Hash>& vec1 = config1.get<vector<Hash> >("Format.elements");
    CPPUNIT_ASSERT(vec1.size() == 1);
    CPPUNIT_ASSERT(vec1[0].has("UINT32") == true);
    CPPUNIT_ASSERT(vec1[0].has("UINT32.h5path") == true);
    CPPUNIT_ASSERT(vec1[0].has("UINT32.h5name") == true);
    CPPUNIT_ASSERT(vec1[0].has("UINT32.key") == true);
    CPPUNIT_ASSERT(vec1[0].has("UINT32.compressionLevel") == true);


    h5::Element::Pointer p1 = format->getElement("experimental.test23");
    CPPUNIT_ASSERT(p1->getH5name() == "test23");
    CPPUNIT_ASSERT(p1->getKey() == "instrument.test");
    CPPUNIT_ASSERT(p1->getFullName() == "experimental/test23");
    CPPUNIT_ASSERT(p1->getMemoryType() == Types::UINT32);
    CPPUNIT_ASSERT(p1->getDims().rank() == 0);
    CPPUNIT_ASSERT(p1->getDims().size() == 0);


    Hash config1p;
    format->getPersistentConfig(config1p);

    KARABO_LOG_FRAMEWORK_TRACE << "config2:\n" << config1p;

    const vector<Hash>& vec1p = config1p.get<vector<Hash> >("Format.elements");
    CPPUNIT_ASSERT(vec1p.size() == 1);
    CPPUNIT_ASSERT(vec1p[0].has("UINT32") == true);
    CPPUNIT_ASSERT(vec1p[0].has("UINT32.h5path") == true);
    CPPUNIT_ASSERT(vec1p[0].has("UINT32.h5name") == true);
    CPPUNIT_ASSERT(vec1p[0].has("UINT32.key") == false);
    CPPUNIT_ASSERT(vec1p[0].has("UINT32.compressionLevel") == true);



    {
        Hash c2(
                "h5path", "experimental2",
                "h5name", "test1000",
                "key", "instrument.test2",
                "compressionLevel", 0,
                "dims", Dims(10, 10).toVector(),
                "type", "VECTOR_INT32"
                );

        h5::Element::Pointer e2 = h5::Element::create("VECTOR_INT32", c2);
        format->replaceElement("experimental.test23", e2);

        h5::Element::Pointer p1 = format->getElement("experimental2.test1000");
        CPPUNIT_ASSERT(p1->getH5name() == "test1000");
        CPPUNIT_ASSERT(p1->getKey() == "instrument.test2");
        CPPUNIT_ASSERT(p1->getFullName() == "experimental2/test1000");
        CPPUNIT_ASSERT(p1->getMemoryType() == Types::VECTOR_INT32);
        CPPUNIT_ASSERT(p1->getDims().rank() == 2);
        CPPUNIT_ASSERT(p1->getDims().extentIn(0) == 10);
        CPPUNIT_ASSERT(p1->getDims().extentIn(1) == 10);
        CPPUNIT_ASSERT(p1->getDims().size() == 100);


        const Hash config2 = format->getConfig();

        const vector<Hash>& vec2 = config2.get<vector<Hash> >("Format.elements");
        CPPUNIT_ASSERT(vec2.size() == 1);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.h5path") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.h5name") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.key") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.compressionLevel") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.dims") == true);




        Hash config2p;
        format->getPersistentConfig(config2p);

        const vector<Hash>& vec2p = config2p.get<vector<Hash> >("Format.elements");
        CPPUNIT_ASSERT(vec2p.size() == 1);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.h5path") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.h5name") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.key") == false);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.compressionLevel") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.dims") == true);
    }


    {

        Hash c2(
                "h5path", "experimental2",
                "h5name", "test1000",
                "key", "instrument.test2",
                "compressionLevel", 0,
                "dims", Dims(10, 10).toVector(),
                "type", "PTR_INT32"

                );

        h5::Element::Pointer e2 = h5::Element::create("VECTOR_INT32", c2);
        format->replaceElement("experimental.test23", e2);

        const Hash config2 = format->getConfig();

        const vector<Hash>& vec2 = config2.get<vector<Hash> >("Format.elements");
        CPPUNIT_ASSERT(vec2.size() == 1);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.h5path") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.h5name") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.key") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.compressionLevel") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.dims") == true);
        CPPUNIT_ASSERT(vec2[0].has("VECTOR_INT32.type") == true);


        Hash config2p;
        format->getPersistentConfig(config2p);

        const vector<Hash>& vec2p = config2p.get<vector<Hash> >("Format.elements");
        CPPUNIT_ASSERT(vec2p.size() == 1);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.h5path") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.h5name") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.key") == false);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.compressionLevel") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.dims") == true);
        CPPUNIT_ASSERT(vec2p[0].has("VECTOR_INT32.type") == false);
    }



    format->removeElement("experimental2.test1000");

    const Hash config3 = format->getConfig();
    CPPUNIT_ASSERT(config3.has("Format") == true);
    CPPUNIT_ASSERT(config3.is<Hash > ("Format") == true);
    CPPUNIT_ASSERT(config3.has("Format.elements") == true);
    CPPUNIT_ASSERT(config3.get<vector<Hash> >("Format.elements").size() == 0);

    //    clog << endl << "config3:" << endl << config3 << endl;

}


void H5Format_Test::testDiscoverFromHash() {


    {


        try {

            Hash data;

            vector<int> vecInt(100, 2);
            data.set("a.b.x", vecInt);
            addPointerToHash(data, "a.b.y", &vecInt[0], Dims(2, 5, 10));
            data.set("b.b1", 123);
            data.set("b.b2", 123u);
            data.set("b.b3", 123LL);
            data.set("b.b4", std::complex<float>(2, 6));
            data.set("b.b5", true);
            data.set("c.c1", Hash());
            data.set("c.c2", vector<Hash>(4, Hash()));




            Format::Pointer dataFormat = Format::discover(data);


            KARABO_LOG_FRAMEWORK_TRACE_CF << "config\n" << dataFormat->getConfig();

            Hash pers;
            dataFormat->getPersistentConfig(pers);

            KARABO_LOG_FRAMEWORK_TRACE_CF << "persistent config\n" << pers;

            vector<std::string> names;
            dataFormat->getElementsNames(names);
            for (size_t i = 0; i < names.size(); ++i) {
                //  clog << "names[" << i << "] = " << names[i] << endl;
                h5::Element::Pointer el = dataFormat->getElement(names[i]);
                //  clog << "Memory type: " << Types::to<ToLiteral>(el->getMemoryType() ) << endl;
                // clog << "Element type: " << el->getElementType()  << endl;
                Dims dims = el->getDims();
                //  clog << ":size " << dims.size()  << endl;
            }

        } catch (const Exception& e) {
            std::clog << e.detailedMsg() << std::endl;
            KARABO_RETHROW
        }

    }


}


