/*
 * File:   NDArray_Test.cc
 * Author: parenti
 *
 * Created on May 22, 2015, 11:43:22 AM
 */

#include <boost/thread/pthread/thread_data.hpp>

#include "NDArray_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::net;
using namespace karabo::xms;

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
    
    Dims dims(100, 200); // height, width
    std::vector<int> someData(dims.size(), 2);
    
    {
        NDArray ar(&someData[0], someData.size(), true, dims);
        
        Dims arDims(ar.getDimensions());
        
        CPPUNIT_ASSERT(ar.getByteSize() == 20000 * sizeof (int));
        
        CPPUNIT_ASSERT(arDims.size() == 20000);
        CPPUNIT_ASSERT(arDims.rank() == 2);
        CPPUNIT_ASSERT(arDims.x1() == 100);
        CPPUNIT_ASSERT(arDims.x2() == 200);
    }
    
}

void NDArray_Test::testSetAndGetMethods() {
    
    Dims dims(100, 200); // height, width
    int tmp[] = {Dimension::STACK, Dimension::DATA};
    std::vector<int> dimTypes(tmp, tmp+2);
    std::vector<int> someData(dims.size(), 2);
    
    {
        NDArray ar;
        
        // Set
        ar.setData(&someData[0], someData.size(), true);
        ar.setDimensions(dims);
        ar.setDimensionTypes(dimTypes);
        
        // Get
        Dims arDims(ar.getDimensions());
        std::vector<int> arDimTypes = ar.getDimensionTypes();
        
        CPPUNIT_ASSERT(arDims.rank() == 2);
        CPPUNIT_ASSERT(arDims.x1() == 100);
        CPPUNIT_ASSERT(arDims.x2() == 200);
        
        CPPUNIT_ASSERT(arDimTypes.size() == 2);
        CPPUNIT_ASSERT(arDimTypes[0] == Dimension::STACK);
        CPPUNIT_ASSERT(arDimTypes[1] == Dimension::DATA);
         
    }
    
}