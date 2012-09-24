/*
 * $Id: testHdf5BufferWriter.cc 5644 2012-03-20 13:17:40Z jszuba $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <boost/shared_array.hpp>
#include <exfel/util/Test.hh>
#include <exfel/util/Time.hh>
#include <exfel/util/Hash.hh>
#include "../hdf5/Table.hh"
#include "../hdf5/File.hh"
#include "../ArrayView.hh"
#include "../Writer.hh"
#include "../Reader.hh"
#include "../hdf5/FLArrayFilter.hh"
#include "../hdf5/TypeTraits.hh"
#include "../hdf5/Column.hh"
#include "../hdf5/WriteBuffer.hh"
#include <exfel/util/Profiler.hh>


#define report(p,name) std::cout << name << ": " << exfel::util::HighResolutionTimer::time2double(p.getTime(name)) << std::endl


#ifdef USE_TRACER
#define tracer if(0); else std::cerr
#else 
#define tracer if(1); else std::cerr
#endif


using namespace std;
using namespace exfel::util;
using namespace exfel::io;
using namespace exfel::io::hdf5;

#define NX 1024 //2 //200 //1024
#define NY 1024 //3 //200 //1024
#define NUM_TRAINS 5


static vector<size_t> trains(NUM_TRAINS);

size_t read(unsigned short* ptr, size_t max, size_t trainNumber) {

    unsigned short n = trainNumber;

    size_t nElements = NX * NY;

    size_t ret = max;
    for (size_t i = 0; i < max; ++i) {
        for (size_t j = 0; j < nElements; ++j) {
            ptr[i * nElements + j] = n++;
        }
        if (i == trains[trainNumber] - 1) {
            ret = trains[trainNumber];
            break;
        }
    }
    return ret;
}

size_t readHardware(unsigned int* hardwareArray, size_t max, size_t trainNumber) {

    short amplitude;
    unsigned char flag;

    short n = trainNumber;
    size_t ret = max;

    for (size_t i = 0; i < max; ++i) {
        amplitude = n++;
        tracer << "amplitude: " << amplitude << endl;
        flag = 0;
        if (!(i % 4)) {
            flag = 1;
        }
        hardwareArray[i] = (amplitude << 16) | flag;
        tracer << "hardwareArray: " <<  hardwareArray[i] << endl;
        if (i == trains[trainNumber] - 1) {
            ret = trains[trainNumber];
            break;
        }
    }
    return ret;
}

int testHdf5BufferWriter(int argc, char** argv) {
    return 0;

    try {

        Test t;
        TEST_INIT(t, argc, argv);

        cout << t << endl;

        typedef unsigned short PixelType;


        size_t bufSize = 100;
        size_t numTrains = NUM_TRAINS;
        size_t maxRecords = 4096;


        trains[0] = 14;
        trains[1] = 30;
        trains[2] = 48;
        trains[3] = 15;
        trains[4] = 405;


        string filename = "/dev/shm/bufferWriter.h5";
        //filename = "bufferWriter.h5";

        Profiler p("apd");

        {
            p.start("hdf5 preparation");

            ArrayDimensions dims(NX, NY);

            File file(t.file(filename));
            file.open(File::TRUNCATE);

            hdf5::WriteBuffer buffer(file, "/apd", bufSize);

            size_t idx_amp = buffer.defineColumn<short>("amplitude");
            size_t idx_flag = buffer.defineColumn<unsigned char>("flag");
            size_t idx_image = buffer.defineArrayColumn<PixelType > ("image", dims);
            buffer.commitDefinition();
            p.stop();



            // define chunk size and buffer size to be equal
            // read images from detector can be anything between 0 - 512
            // read amplitude and flag


            cout << "Max. number of read records: " << maxRecords << endl;
            cout << "Buffer size                : " << bufSize << endl;

            ArrayDimensions readBufferImageDims(maxRecords, NX, NY);
            ArrayView<PixelType> readBufferArrayView(readBufferImageDims);
            PixelType* ptr = &readBufferArrayView[0];

            ArrayDimensions readBufferScalarsDims(maxRecords);
            ArrayView<unsigned int> readBufferScalars(readBufferScalarsDims);
            unsigned int* scalarPtr = &readBufferScalars[0];

            size_t numRecords = 0;
            for (size_t i = 0; i < numTrains; ++i) {


                p.start("read");
                numRecords = read(ptr, maxRecords, i);
                tracer << "Number of read records     : " << numRecords << endl;
                numRecords = readHardware(scalarPtr, maxRecords, i);
                tracer << "Number of read records     : " << numRecords << endl;
                p.stop();
                p.start("writing");


                size_t m = 0;
                for (size_t j = 0; j < numRecords; ++j, buffer.next()) {                                        
                    tracer << "sc: " << scalarPtr[j] << endl;
                    unsigned char flag = scalarPtr[j] & 0x00ff;
                    buffer.set<unsigned char>(idx_flag, flag);
                    short amplitude = scalarPtr[j] >> 16;
                    buffer.set<short>(idx_amp, amplitude);
                    //                    tracer << "m                      : " << m << endl;
                    ArrayView<PixelType> a(&ptr[m], dims);
                    buffer.setArray(idx_image, a);
                    m += dims.getNumberOfElements();
                }
                // make sure that the data are flushed before reading the next portion to the same area of memory.
                // note ptr in:  read(ptr, maxRecords, i);                
                buffer.flush();
                p.stop();
            }

            report(p, "hdf5 preparation");
            report(p, "read");
            report(p, "writing");

        }

        {



            File fileRead(t.file(filename));
            fileRead.open(File::READONLY);

            Table::Pointer tableRead = fileRead.getTable("/apd");

            Column<short> amp("amplitude", tableRead);
//            Column<int> x("x", tableRead);
//            Column<int> y("y", tableRead);
//            Column<int> z("z", tableRead);
            //	    Column<ArrayView<PixelType> > array("array", tableRead);


            long long t3 = exfel::util::Time::getMsSinceEpoch();
            size_t nRecords = tableRead->getNumberOfRecords();
            //nRecords = 10;
            for (size_t i = 0; i < nRecords; ++i) {
                tracer << "i: " << i << " amplitude=" << amp[i] << endl;
            }
            long long t4 = exfel::util::Time::getMsSinceEpoch();
            tracer << "reading time: " << (t4 - t3) << " [ms]" << endl;

            fileRead.close();
        }

        
        //            bool write = false;
        //            if (write) {
        //
        //
        //                File file(t.file(filename));
        //                file.open(File::TRUNCATE);
        //
        //                vector<int> intVector(bufSize, 1);
        //                ArrayView<int> intBuffer(intVector);
        //                for (size_t i = 0; i < intBuffer.getSize(); ++i) {
        //                    intBuffer[i] = i + pid;
        //                }
        //
        //                // array has rank=2
        //                size_t nx = 1; //1024;//1; //1024;
        //                size_t ny = 2; //1024;//; //1024;
        //
        //
        //                // array data to be written, 
        //                // contiguous memory block is required for buffered writing
        //                boost::shared_array<PixelType> arr(new PixelType[ nx * ny * bufSize]);
        //                for (size_t i = 0; i < (nx * ny * bufSize); ++i) {
        //                    arr[i] = i;
        //                }
        //
        //
        //
        //                // This hash will contain vectors
        //                // Each vector element will be mapped to a record in the Table
        //                Hash bufData;
        //
        //                // vectors with arrays
        //                // for efficiency we bind reference
        //                vector< ArrayView<PixelType> >& vec = bufData.bindReference<vector< ArrayView<PixelType> > >("array");
        //                //	    vector< ArrayView<PixelType> > vec;
        //
        //                // first interpret the whole memory block as Array[bufSize,nx,ny]
        //                ArrayDimensions arrayDims(bufSize, nx, ny);
        //                ArrayView<PixelType> av(arr, arrayDims);
        //
        //                // convert to vector of arrays [nx,ny]
        //                // remember that vec is already a reference to the vector inside the Hash
        //                av.getVectorOfArrayViews(vec);
        //
        //
        //                tracer << vec[0][0] << " ";
        //                tracer << vec[0][1] << " ";
        //                tracer << vec[0][2] << " " << endl;
        //
        //
        //                tracer << vec[2][0] << " ";
        //                tracer << vec[2][1] << " ";
        //                tracer << vec[2][2] << " " << endl;
        //
        //                // now the rest of data, 
        //                bufData.set("abc", intBuffer);
        //                bufData.set("x", intBuffer);
        //                bufData.set("y", intBuffer);
        //                bufData.set("z", intBuffer);
        //                bufData.set("a1", intBuffer);
        //                bufData.set("a2", intBuffer);
        //                bufData.set("a3", intBuffer);
        //                bufData.set("a4", intBuffer);
        //                bufData.set("a5", intBuffer);
        //
        //
        //                // this one contains single record, currently needed for format discovery
        //                Hash data;
        //                data.set("abc", 0);
        //                data.set("x", 0);
        //                data.set("y", 0);
        //                data.set("z", 0);
        //                data.set("array", vec[0]);
        //                data.set("a1", 0);
        //                data.set("a2", 0);
        //                data.set("a3", 0);
        //                data.set("a4", 0);
        //                data.set("a5", 0);
        //
        //                DataFormat::Pointer dataFormat, dataFormat1;
        //                Hash dfc;
        //                bool discoverConfig = true;
        //                if (discoverConfig) {
        //                    try {
        //                        dataFormat = DataFormat::discoverFromData(data);
        //                        dataFormat1 = DataFormat::discoverFromData(bufData);
        //                    } catch (...) {
        //                        RETHROW;
        //                    }
        //                    dfc = dataFormat->getConfig();
        //                    tracer << "dataFormatConfig: " << endl << dfc << endl;
        //                    Writer<Hash>::Pointer wc = Writer<Hash>::create(Hash("TextFile.filename", t.file("bufferWriter.xml")));
        //                    wc->write(dfc);
        //                } else {
        //                    Reader<Hash>::Pointer rc = Reader<Hash>::create(Hash("TextFile.filename", t.file("bufferWriterConv.xml")));
        //                    rc->read(dfc);
        //                }
        //                dataFormat = DataFormat::create(dfc);
        //
        //
        //                tracer << "-----" << endl << data << endl << "-----";
        //
        //                // create table
        //                Table::Pointer table = file.createTable("/test", dataFormat, bufSize);
        //                Table::Pointer table1 = file.createTable("/test1", dataFormat1);
        //                tracer << "table created " << endl;
        //
        //                long long t1 = exfel::util::Time::getMsSinceEpoch();
        //                // write buffer 
        //                for (size_t i = 0; i < nBuffers; ++i) {
        //                    table->writeBuffer(bufData, i * intBuffer.getSize(), intBuffer.getSize());
        //                }
        //                long long t2 = exfel::util::Time::getMsSinceEpoch();
        //                tracer << "Writing time: " << (t2 - t1) << " [ms]" << endl;
        //                sleep(2);
        //                //	    long long t3 = exfel::util::Time::getMsSinceEpoch();
        //                //	    for (int i = 0; i < nBuffers; ++i) {
        //                //		table1->write(bufData, i);
        //                //	    }
        //                //	    long long t4 = exfel::util::Time::getMsSinceEpoch();
        //                //	    tracer << "Writing time: " << (t4 - t3) << " [ms]" << endl;
        //
        //
        //
        //
        //                file.close();
        //
        //                sleep(4);
        //
        //
        //            }
        //        {
        //
        //
        //
        //            File fileRead(t.file(filename));
        //            fileRead.open(File::READONLY);
        //
        //            Table::Pointer tableRead = fileRead.getTable("/test");
        //
        //            Column<int> abc("abc", tableRead);
        //            Column<int> x("x", tableRead);
        //            Column<int> y("y", tableRead);
        //            Column<int> z("z", tableRead);
        //            //	    Column<ArrayView<PixelType> > array("array", tableRead);
        //
        //
        //            long long t3 = exfel::util::Time::getMsSinceEpoch();
        //            size_t nRecords = tableRead->getNumberOfRecords();
        //            //nRecords = 10;
        //            for (size_t i = 0; i < nRecords; ++i) {
        //
        //                int abcValue = abc[i];
        //                int xValue = 0;
        //                xValue = x[i];
        //                //if (i > 0) {
        //                //    xValue = x[i - 1];
        //                //}
        //                int zValue = y[i];
        //                int yValue = z[i];
        //
        //                //		ArrayView<PixelType> aView = array[i];
        //                if (!((i - 54) % (nRecords / 10))) {
        //                    tracer << "i: " << i << endl;
        //                    //		    tracer << "data abc: " << abcValue << endl;
        //                    //		    tracer << "Rank: " << aView.getNumDims() << endl;
        //                    tracer << "data x  : " << xValue << endl;
        //                    tracer << "data z  : " << zValue << endl;
        //                }
        //            }
        //            long long t4 = exfel::util::Time::getMsSinceEpoch();
        //            tracer << "reading time: " << (t4 - t3) << " [ms]" << endl;
        //
        //            fileRead.close();
        //        }

    } catch (Exception e) {
        cout << e;
        RETHROW
    }
    return 0;
}

