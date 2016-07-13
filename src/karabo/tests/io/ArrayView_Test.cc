/* 
 * File:   ArrayView_Test.cc
 * Author: <krzysztof.wrona@xfel.eu>
 * 
 * Created on November 27, 2012, 1:37 PM
 */

#include "ArrayView_Test.hh"

using namespace karabo::io;
using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION(ArrayView_Test);


ArrayView_Test::ArrayView_Test() {

}


ArrayView_Test::~ArrayView_Test() {
}


void ArrayView_Test::setUp() {
}


void ArrayView_Test::tearDown() {
}


void ArrayView_Test::testCArray() {

    // "a" is a 2-dim array (any type):
    // 0   1   2   3 
    // 4   5   6   7
    // 8   9  10  11
    //
    //  array must be stored as continues block in the memory and the number of elements (size) cannot be changed.
    //  ArrayView does not own the data, only the pointer to the memory block
    //  This is needed for performance reasons to avoid copying large amount of data block
    //  
    //  size of the array is 12
    //  number of dimensions (rank) is 2
    //  number of rows is 3
    //  number of columns is 4
    //  
    // 


    size_t size = 12;
    long long* rawLongLong = new long long[size];
    for (size_t i = 0; i < size; ++i) {
        rawLongLong[i] = i * 5;
    }

    int ndims = 2;
    size_t dims[2] = {3, 4};

    ArrayView<long long> a(rawLongLong, ndims, dims);
    tracer << a.getNumDims() << endl;
    CPPUNIT_ASSERT(a.getNumDims() == 2);
    ArrayDimensions sizes = a.getDims();
    CPPUNIT_ASSERT(sizes[0] == 3);
    CPPUNIT_ASSERT(sizes[1] == 4);
    tracer << "ArrayDimensions.size() = " << sizes.size() << endl;

    for (size_t i = 0; i < size; ++i) {
        CPPUNIT_ASSERT(a[i] == i * 5);
        tracer << "[" << i << "]: " << a[i] << ", ";
    }
    tracer << endl;

}


void ArrayView_Test::testIndexable() {

    // allocate vector
    size_t size = 12;
    vector<unsigned int> vecData(size, 0);
    CPPUNIT_ASSERT(vecData.size() == 12);

    // fill the vector with numbers
    for (size_t i = 0; i < vecData.size(); ++i) {
        vecData[i] = i * 10;
    }

    // define how we want to see our data -> 3 x 4 matrix
    ArrayDimensions vecDims(3, 4);
    CPPUNIT_ASSERT(vecDims[0] == 3);
    CPPUNIT_ASSERT(vecDims[1] == 4);

    // using self calculated index
    for (size_t i = 0; i < vecDims[0]; ++i) {
        for (size_t j = 0; j < vecDims[1]; ++j) {
            tracer << vecData[vecDims[1] * i + j] << " ";
            CPPUNIT_ASSERT(vecData[vecDims[1] * i + j] == (vecDims[1] * i + j)*10);
        }
        tracer << endl;
    }


    tracer << "number of elements: " << vecData.size() << endl << " dim[0]: " << vecDims[0] << " dim[1]: " << vecDims[1] << endl;
    tracer << "rank " << vecDims.size() << endl;


    // Define ArrayView using vector data and dimensions
    ArrayView<unsigned int> b(vecData, vecDims);
    CPPUNIT_ASSERT(b.getNumDims() == 2);

    // get the array view dimensions as defined by constructor
    ArrayDimensions bSizes = b.getDims();
    tracer << "dimensions [0]: " << bSizes[0] << " [1]: " << bSizes[1] << endl;
    CPPUNIT_ASSERT(bSizes[0] == 3);
    CPPUNIT_ASSERT(bSizes[1] == 4);


    // look at vector as array view of array views: [3 x 4]
    // this allow us to access elements by double index, i.e.: b2[0][1], b2[0][2], ..., b2[2][3]
    ArrayView< ArrayView<unsigned int> > b2 = b.indexable();

    tracer << "number of dimensions: " << b2.getNumDims() << endl;
    CPPUNIT_ASSERT(b2.getNumDims() == 1);
    tracer << "number of columns: " << b2.getSize() << endl;
    CPPUNIT_ASSERT(b2.getSize() == 3);

    tracer << "number of rows: " << b2[0].getSize() << endl;
    CPPUNIT_ASSERT(b2[0].getSize() == 4);
    CPPUNIT_ASSERT(b2[1].getSize() == 4);
    CPPUNIT_ASSERT(b2[2].getSize() == 4);

    for (size_t i = 0; i < b2.getSize(); ++i) {
        for (size_t j = 0; j < b2[i].getSize(); ++j) {
            tracer << b2[i][j] << " ";
            CPPUNIT_ASSERT(b2[i][j] == (i * b2[i].getSize() + j)*10);
        }
        tracer << endl;
    }
}

