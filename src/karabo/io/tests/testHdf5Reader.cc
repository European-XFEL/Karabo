/*
 * $Id: testHdf5Reader.cc 5503 2012-03-12 08:00:04Z wegerk $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <exfel/util/Test.hh>
#include "../hdf5/Table.hh"
#include <exfel/util/CArray.hh>
#include "../ArrayView.hh"
#include "../CImg.h"
#include "../hdf5/FLArrayFilter.hh"

using namespace std;
using namespace exfel::util;
using namespace exfel::io;
using namespace exfel::io::hdf5;

int testHdf5Reader(int argc, char** argv) {

  try {

    Test t;
    TEST_INIT(t, argc, argv);

    cout << t << endl;
    // use t.file("filename"); to access file

    std::cout << "FLArrayFilter<float> registry: \n";
    std::cout << GenericFactory< Factory<FLArrayFilter<float> > >::getInstance().getKeysAsString();

    Hash data1, data2, data3, header;
    int c = 12;
    double d = 0.125;
    unsigned short us = 20;

    int nrows = 2;
    int ncols = 4;
    int npix = nrows*ncols;
    bool print = true;
    bool show = false;
    float arrRaw[ npix];
    for (int i = 0; i < npix; ++i) {
      arrRaw[i] = i + 0.1;
    }


    //    CImg<float> imageOrig(arrRaw,nrows,ncols);
    //    imageOrig.display();
    //    float* pb = imageOrig.data();
    //    for (int i = 0; i < npix; ++i) {
    //      pb[i] = 100 + i%ncols;
    //    }
    //    imageOrig.display();

    ArrayView<float> arr(arrRaw, nrows, ncols);


    data1.setFromPath("a1.db1.c", c);
    data1.setFromPath("a1.db1.d", d);
    data1.setFromPath("a1.db2.f", c);
    data1.setFromPath("a1.db2.g", d);
    data1.setFromPath("a1.db3.us", us);
    data1.setFromPath("a2.db1.c", 2 * c);
    data1.setFromPath("a2.db1.d", 2 * d);
    data1.setFromPath("a2.db4.arr", arr);



    const int arrayStringLen = 6;
    std::string arrayString[arrayStringLen];

    std::string s0 = "amamamamamamamam";
    std::string s1 = "psapsapsapsapsapsapsapsapsapsapsapsapsapsa";
    std::string s2 = "K";
    std::string s3 = "aaaaa%%%%aaaaa";
    std::string s4 = "";
    std::string s5 = "note that previous string is empty";

    arrayString[0] = s0;
    arrayString[1] = s1;
    arrayString[2] = s2;
    arrayString[3] = s3;
    arrayString[4] = s4;
    arrayString[5] = s5;


    header.setFromPath("user.Run", 220);
    header.setFromPath("user.Instrument", "SPB");

    std::vector<std::string> vecs(5, "Hello from Mars!!!");
    vecs[2] = "";
    vecs[3] = "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";

    std::vector<float> vecf(5, 1.2345);

    ArrayView < std::string> avStr(arrayString, arrayStringLen);
    for (int i = 0; i < arrayStringLen; ++i) {
      assert(arrayString[i] == avStr[i]);
    }

    header.setFromPath("user.vectorOfStrings", vecs);
    header.setFromPath("user.arrayOfStrings", avStr);
    header.setFromPath("user.vectorOfFloats", vecf);

    DataFormat::Pointer dataFormat = DataFormat::discoverFromData(data1);
    DataFormat::Pointer headerFormat = DataFormat::discoverFromData(header);

    Hash confFile;
    confFile.setFromPath("Hdf5.filename", t.file("hdf5Reader.h5"));

    File::Pointer file = File::create(confFile);
    file->open(File::TRUNCATE);
    Table::Pointer tableHeader = file->createTable("/Header", headerFormat);
    tableHeader->append(header);
    Table::Pointer table = file->createTable("/RawData", dataFormat);
    for (int i = 0; i < 2; ++i) {
      table->append(data1);
    }
    file->close();




    // READING with default memory buffers

    file->open(File::READONLY);

    Table::Pointer headerTable = file->getTable("/Header");
    Hash headerRead;
    headerTable->allocate(headerRead);
    headerTable->read(headerRead, 0);
    cout << "Header: " << headerRead << endl;

    ArrayView < std::string> & strArr = headerRead.getFromPath<ArrayView<std::string > > ("user.arrayOfStrings");
    assert(strArr.getNumDims() == 1);
    std::cout << "dims[0] = " << strArr.getDims()[0] << std::endl;
    assert(strArr.getDims()[0] == static_cast<size_t> (arrayStringLen));

    for (size_t i = 0; i < strArr.getDims()[0]; ++i) {
      assert(strArr[i] == arrayString[i]);
      cout << "string array[" << i << "] = " << strArr[i] << endl;      
    }

    // By default arrays are read to memory buffer and are accessible using ArrayView interface
    // This is of course independent on the way how they were written to a file
    strArr = headerRead.getFromPath<ArrayView<std::string > > ("user.vectorOfStrings");
    assert(strArr.getNumDims() == 1);
    assert(strArr.getDims()[0] == vecs.size());
    for (size_t i = 0; i < strArr.getDims()[0]; ++i) {
      assert(strArr[i] == vecs[i]);
      //cout << "string array[" << i << "] = " << strArr[i] << endl;
    }


    Table::Pointer rawDataTable = file->getTable("/RawData");

    Hash attributes;
    rawDataTable->readAttributes(attributes);

    cout << "Attributes: \n" << attributes << endl;

    Hash dataRead;
    rawDataTable->allocate(dataRead);
    cout << "allocated: " << dataRead << endl;
    for (size_t i = 0; i < rawDataTable->getNumberOfRecords(); ++i) {

      rawDataTable->read(dataRead, i);
      cout << "record[" << i << "]: \n" << dataRead << endl;
      ArrayView<float>& av = dataRead.getFromPath<ArrayView<float> >("a2.db4.arr");
      ArrayDimensions adim = av.getDims();
      //      imageOrig.assign(&av[0],adim[0],adim[1]);
      //      imageOrig.display();
      cout << "a2.db4.arr => " << endl;
      if (print) {
        int ndims = av.getNumDims();
        if (ndims == 1) {
          for (size_t i = 0; i < av.getDims()[0]; ++i) {
            cout << av[i] << " ";
          }
          cout << endl;
        }
        if (ndims == 2) {
          for (size_t i = 0; i < av.getDims()[0]; ++i) {
            for (size_t j = 0; j < av.getDims()[1]; ++j) {
              cout << setw(6) << av[j + i * av.getDims()[1]] << " ";
              if (j == 10) break;
            }
            cout << endl;
            if (i == 10) break;
          }
        }
      }
    }


    exit(0);
    

    // READING with memory buffers managed by client (us).

    attributes.clear();
    headerTable->readAttributes(attributes);

    ArrayDimensions dims = attributes.getFromPath<ArrayDimensions > ("user.vectorOfStrings.dims");
    ArrayDimensions dimsF = attributes.getFromPath<ArrayDimensions > ("user.vectorOfFloats.dims");
    std::vector<std::string> readVector(dims[0], "");
    std::vector<double > readFasDVector(dimsF[0], 0);


    Hash storage;
    storage.setFromPath("user.vectorOfStrings", readVector);
    storage.setFromPath("user.vectorOfFloats", readFasDVector);
    headerTable->allocate(storage);
    headerTable->read(storage, 0);

    std::vector< std::string> & strVec = storage.getFromPath<std::vector<std::string> > ("user.vectorOfStrings");
    assert(vecs.size() == strVec.size());
    for (size_t i = 0; i < vecs.size(); ++i) {
      cout << "string vector[" << i << "] = " << strVec[i] << endl;
      assert(vecs[i] == strVec[i]);
    }

    std::cout << "========== Conversion from vector<float> to vector<double> ==========" << std::endl;
    std::vector< double> & dVec = storage.getFromPath<std::vector<double> > ("user.vectorOfFloats");
    assert(vecf.size() == dVec.size());
    for (size_t i = 0; i < vecf.size(); ++i) {
      cout << "double vector[" << i << "] = " << dVec[i] << endl;
      assert(abs(vecf[i] - dVec[i]) < 0.001);
    }



    storage.clear();
    std::vector<int > readFasI32Vector(dimsF[0], 0);
    storage.setFromPath("user.vectorOfFloats", readFasI32Vector);
    headerTable->allocate(storage);
    headerTable->read(storage, 0);

    std::cout << "========== Conversion from vector<float> to vector<int> ==========" << std::endl;
    std::vector< int> & i32Vec = storage.getFromPath<std::vector<int> > ("user.vectorOfFloats");
    assert(vecf.size() == i32Vec.size());
    for (size_t i = 0; i < vecf.size(); ++i) {
      cout << "int32 vector[" << i << "] = " << i32Vec[i] << endl;
      assert(abs(vecf[i] - i32Vec[i]) < 1);
    }














    cout << "=============== image ===============" << endl;

    attributes.clear();
    rawDataTable->readAttributes(attributes);


    dims = attributes.getFromPath<ArrayDimensions > ("a2.db4.arr.dims");
    cout << "image dimensions: [" << dims[0] << ", " << dims[1] << "]" << endl;

    for (size_t i = 0; i < rawDataTable->getNumberOfRecords(); ++i) {

      cimg_library::CImg<float> image(dims[0], dims[1]);
      Hash externalStorage;
      externalStorage.setFromPath("a2.db4.arr", image.data());
      //vector<float> vec(dims[0]*dims[1],0);
      //externalStorage.setFromPath("a2.db4.arr", vec);   
      rawDataTable->allocate(externalStorage);


      rawDataTable->read(externalStorage, i);
      cout << "record[" << i << "]: \n" << externalStorage << endl;

      //      ArrayView<float>& av = externalStorage.getFromPath<ArrayView<float> >("a2.db4.arr");
      //      cout << "a2.db4.arr => " << endl;
      //      int ndims = av.getNumDims();
      //      if (print) {
      //        if (ndims == 1) {
      //          for (size_t i = 0; i < av.getDims()[0]; ++i) {
      //            cout << av[i] << " ";
      //          }
      //          cout << endl;
      //        }
      //        if (ndims == 2) {
      //          for (size_t i = 0; i < av.getDims()[0]; ++i) {
      //            for (size_t j = 0; j < av.getDims()[1]; ++j) {
      //              cout << setw(6) << av[j + i * av.getDims()[1]] << " ";
      //            }
      //            cout << endl;
      //          }
      //        }
      //      }
      if (show) {
        std::cout << "Showing images read from file using client preallocated memory [image.data()]" << std::endl;
        if (dims.size() == 2) {
          image.display();
        }
      }
    }





    file->close();

    return 0;





    cout << "--------- table2 ---------" << endl;
    confFile.setFromPath("Hdf5.filename", t.file("discoverFromData2.h5"));
    File::Pointer file2 = File::create(confFile);
    file2->open(File::TRUNCATE);


    data2.setFromPath("a", c);
    DataFormat::Pointer dataFormat2 = DataFormat::discoverFromData(data2);


    Table::Pointer table2 = file2->createTable("/", dataFormat2);
    for (int i = 0; i < 2; ++i) {
      table2->append(data2);
    }
    file2->close();

    cout << "--------- table3 ---------" << endl;
    confFile.setFromPath("Hdf5.filename", t.file("discoverFromData3.h5"));
    File::Pointer file3 = File::create(confFile);
    file3->open(File::TRUNCATE);


    data3.setFromPath("a.b", c);
    DataFormat::Pointer dataFormat3 = DataFormat::discoverFromData(data3);

    Table::Pointer table3 = file3->createTable("/", dataFormat3);
    for (int i = 0; i < 3; ++i) {
      table3->append(data3);
    }
    file3->close();


  } catch (Exception e) {
    cout << e;
    RETHROW
  }

  return 0;
}

