/*
 * File:   NDArray_Test.cc
 * Author: parenti
 *
 * Created on May 22, 2015, 11:43:22 AM
 */

#include <boost/thread/pthread/thread_data.hpp>
#include <karabo/util/StringTools.hh>
#include <karabo/util/NDArray.hh>
#include <cppunit/TestAssert.h>

#include "NDArray_Test.hh"

using namespace std;
using namespace karabo::util;

CPPUNIT_TEST_SUITE_REGISTRATION(NDArray_Test);


NDArray_Test::NDArray_Test() {
}


NDArray_Test::~NDArray_Test() {
}


void NDArray_Test::setUp() {
}


void NDArray_Test::tearDown() {
}


void NDArray_Test::testConstructor() {

    const Dims shape(100, 200);
    vector<int> someData(100*200, 2);

    Hash h;

    {
        NDArray fly(shape, 2);
        NDArray cpy(&someData[0], someData.size(), shape);
        NDArray ref(&someData[0], someData.size(), NDArray::NullDeleter(), shape);

        const Dims& flyShape = fly.getShape();
        const Dims& cpyShape = cpy.getShape();
        const Dims& refShape = ref.getShape();

        CPPUNIT_ASSERT(flyShape.x1() == 100);
        CPPUNIT_ASSERT(flyShape.x2() == 200);
        CPPUNIT_ASSERT(fly.getData<int>()[124] == 2);
        CPPUNIT_ASSERT(fly.size() == 100 * 200);
        CPPUNIT_ASSERT_EQUAL(sizeof (int), fly.itemSize());
        CPPUNIT_ASSERT_EQUAL(sizeof (int) * static_cast<size_t>(100 * 200), fly.byteSize());

        CPPUNIT_ASSERT(cpyShape.x1() == 100);
        CPPUNIT_ASSERT(cpyShape.x2() == 200);
        CPPUNIT_ASSERT(cpy.getData<int>()[124] == 2);
        CPPUNIT_ASSERT(cpy.size() == 100 * 200);
        CPPUNIT_ASSERT_EQUAL(sizeof(int), cpy.itemSize());
        CPPUNIT_ASSERT_EQUAL(sizeof (int) * static_cast<size_t>(100 * 200), cpy.byteSize());

        CPPUNIT_ASSERT(refShape.x1() == 100);
        CPPUNIT_ASSERT(refShape.x2() == 200);
        CPPUNIT_ASSERT(ref.getData<int>()[124] == 2);
        CPPUNIT_ASSERT(ref.size() == 100 * 200);
        CPPUNIT_ASSERT_EQUAL(sizeof(int), ref.itemSize());
        CPPUNIT_ASSERT_EQUAL(sizeof (int) * static_cast<size_t>(100 * 200), ref.byteSize());

        cpy.getData<int>()[0] = 0;
        CPPUNIT_ASSERT(someData[0] == 2);

        ref.getData<int>()[0] = 0;
        CPPUNIT_ASSERT(someData[0] == 0);

        h.set("cpy", cpy);
        h.set("ref", ref);
    }   

    NDArray& ref = h.get<NDArray >("ref");
    CPPUNIT_ASSERT(ref.getData<int>()[124] == 2);
    CPPUNIT_ASSERT(ref.getShape().x1() == 100);
    CPPUNIT_ASSERT(ref.size() == 100 * 200);
    std::clog << "Hash with NDArray ...\n" << h << std::endl;
}


void NDArray_Test::testShapeException() {
    vector<int> data(10, -42);
    const Dims badShape(2, 500);

    {
        CPPUNIT_ASSERT_THROW(NDArray(&data[0], data.size(), badShape), karabo::util::ParameterException);
    }
}
