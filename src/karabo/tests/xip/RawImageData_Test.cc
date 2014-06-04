/* 
 * File:   RawImageData_Test.cc
 * Author: heisenb
 * 
 * Created on May 22, 2014, 3:31 PM
 */

#include "RawImageData_Test.hh"
#include <karabo/xip.hpp>
#include <karabo/util.hpp>
#include <karabo/io.hpp>

CPPUNIT_TEST_SUITE_REGISTRATION(RawImageData_Test);

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::xip;

RawImageData_Test::RawImageData_Test() {
}


RawImageData_Test::~RawImageData_Test() {
}




void RawImageData_Test::testConstructor() {

    std::vector<int> someData(10000, 2);

    {
        RawImageData raw(&someData[0], someData.size());
        CPPUNIT_ASSERT(raw.getByteSize() == 10000 * sizeof(int));
        CPPUNIT_ASSERT(raw.getChannelSpace() == ChannelSpace::s_32_4);
        CPPUNIT_ASSERT(raw.getEncoding() == Encoding::GRAY);
        CPPUNIT_ASSERT(raw.getDimensions().size() == 10000);
        CPPUNIT_ASSERT(raw.getDimensions().rank() == 1);
        CPPUNIT_ASSERT(raw.getType() == "INT32");
    }





}


void RawImageData_Test::testPerformance() {

    std::vector<char> someData(1000E06, 1);

    BinarySerializer<RawImageData>::Pointer p = BinarySerializer<RawImageData>::create("Bin");

    {
        TimeProfiler pr("Serialization");
        pr.open();
        std::vector<char> archive;
        RawImageData tgt;

        pr.startPeriod("copy");
        for (size_t i = 0; i < 1; ++i) {
            RawImageData src(&someData[0], someData.size());
            p->save(src, archive);
            p->load(tgt, archive);
        }
        pr.stopPeriod("copy");
        pr.close();

        cout << "\nSerialization time (copy): " << pr.getPeriod("copy").getDuration() << endl;
    }

    {
        TimeProfiler pr("Serialization");
        pr.open();
        std::vector<char> archive;
        RawImageData tgt;

        pr.startPeriod("share");
        for (size_t i = 0; i < 1; ++i) {
            RawImageData src(&someData[0], someData.size(), false);
            p->save(src, archive);
            p->load(tgt, archive);
        }
        pr.stopPeriod("share");
        pr.close();

        cout << "\nSerialization time (share): " << pr.getPeriod("share").getDuration() << endl;
    }
}
