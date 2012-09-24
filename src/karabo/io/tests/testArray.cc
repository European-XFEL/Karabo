/*
 * $Id: testArray.cc 5497 2012-03-09 22:54:11Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <exfel/util/Test.hh>
#include <exfel/util/Hash.hh>
#include "../ArrayView.hh"
#include "../CImg.h"
#include <hdf5/hdf5.h>

#include <boost/multi_array.hpp>



using namespace std;
using namespace exfel::util;
using namespace exfel::io;

int testArray(int argc, char** argv) {


    try {

        Test t;
        TEST_INIT(t, argc, argv);

        cout << t << endl;


        cout << "size_t: " << sizeof (size_t) << " hsize_t: " << sizeof (hsize_t) << endl;
        cout << "size_t: " << numeric_limits<size_t>::max() << " hsize_t: " << numeric_limits<hsize_t>::max() << endl;
        cout << "ulonglong: " << numeric_limits<unsigned long long int>::max() << endl;
        cout << "ulong: " << numeric_limits<unsigned long int>::max() << " uint: " << numeric_limits<unsigned int>::max() << endl;
        cout << "long: " << numeric_limits<long>::max() << " int: " << numeric_limits<int>::max() << endl;



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
        cout << a.getNumDims() << endl;
        assert(a.getNumDims() == 2);
        ArrayDimensions sizes = a.getDims();
        assert(sizes[0] == 3);
        assert(sizes[1] == 4);
        cout << "ArrayDimensions.size() = " << sizes.size() << endl;

        for (size_t i = 0; i < size; ++i) {
            assert(a[i] == i * 5);
            cout << "[" << i << "]: " << a[i] << ", ";
        }
        cout << endl;




        vector<unsigned int> vecData(size);

        ArrayDimensions vecDims(3, 4);
        cout << "vecDims rank " << vecDims.size() << endl;

        for (size_t i = 0; i < vecData.size(); ++i) {
            vecData[i] = i * 10;
        }

        for (size_t i = 0; i < vecDims[0]; ++i) {
            for (size_t j = 0; j < vecDims[1]; ++j) {
                cout << vecData[vecDims[1] * i + j] << " ";
            }
            cout << endl;
        }



        cout << "second " << vecData.size() << " " << vecDims[0] << " " << vecDims[1] << endl;
        cout << "vecDims rank " << vecDims.size() << endl;
        ArrayView<unsigned int> b(vecData, vecDims);

        cout << b.getNumDims() << endl;
        assert(b.getNumDims() == 2);
        ArrayDimensions bSizes = b.getDims();
        cout << "[0]: " << bSizes[0] << " [1]: " << bSizes[1] << endl;
        assert(bSizes[0] == 3);
        assert(bSizes[1] == 4);



        ArrayView< ArrayView<unsigned int> > b2 = b.indexable();

        cout << "b2: " << b2.getNumDims() << endl;

        for (size_t i = 0; i < b2.getSize(); ++i) {
            for (size_t j = 0; j < b2[i].getSize(); ++j) {
                cout << b2[i][j] << " ";
                assert(b2[i][j] == (i * b2[i].getSize() + j)*10);
            }
            cout << endl;
        }




        ArrayDimensions dimsC(2, 3, 4);
        ArrayView<int> c(dimsC);
        for (size_t i = 0; i < c.getSize(); ++i) {
            c[i] = i;
            cout << c[i] << " ";
        }
        cout << endl;

        if (c.getNumDims() == 3) {
            cout << "3-dim array" << endl;
            const size_t rank = 3;
            typedef boost::multi_array_ref<int, rank > array3D;
            typedef array3D::index index;
            array3D arr3d(&c[0], boost::extents[dimsC[0]][dimsC[1]][dimsC[2]]);
            for (index i = 0; i < static_cast<int>(dimsC[0]); ++i) {
                for (index j = 0; j != static_cast<int>(dimsC[1]); ++j) {
                    for (index k = 0; k != static_cast<int>(dimsC[2]); ++k) {
                        arr3d[i][j][k] += 10;
                        cout << "(" << i << "," << j << "," << k << "): " << arr3d[i][j][k] << endl;
                    }
                }
            }
        }


    } catch (Exception e) {
        cout << e;
        RETHROW
    }

    return 0;
}

