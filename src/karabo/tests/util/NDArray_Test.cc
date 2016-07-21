/*
 * File:   NDArray_Test.cc
 * Author: parenti
 *
 * Created on May 22, 2015, 11:43:22 AM
 */

#include <boost/thread/pthread/thread_data.hpp>
#include <karabo/util/StringTools.hh>
#include <karabo/util/NDArray.hh>

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

    NDArrayShapeType shape;
    shape.push_back(100); // height
    shape.push_back(200); // width
    vector<int> someData(100*200, 2);

    {
        NDArray<int> ar(someData, shape);
        NDArrayShapeType const arShape = ar.getShape();

        CPPUNIT_ASSERT(arShape[0] == 100);
        CPPUNIT_ASSERT(arShape[1] == 200);
    }

}
