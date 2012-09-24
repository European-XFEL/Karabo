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
#include <exfel/util/Profiler.hh>


#define report(p,name) std::cout << name << ": " << exfel::util::HighResolutionTimer::time2double(p.getTime(name)) << std::endl

#define USE_TRACER

#ifdef USE_TRACER
#define tracer if(0); else std::cerr
#else 
#define tracer if(1); else std::cerr
#endif


using namespace std;
using namespace exfel::util;
using namespace exfel::io;
using namespace exfel::io::hdf5;






int testHdf5Train(int argc, char** argv) {
    
 return 0;
    try {

        Test t;
        TEST_INIT(t, argc, argv);

        cout << t << endl;

        typedef unsigned short PixelType;


        size_t bufSize = 512;
	int pid = getpid();

        string filename = "/dev/shm/train.h5";
        
        filename = "train.h5";

        {
            size_t nBuffers = 40;

            
            // prepare buffer for int32 numbers and fill it with dummy values
            boost::shared_array<int> intArr(new int[bufSize]);
            for (size_t i = 0; i < bufSize; ++i) {
                intArr[i] = i + pid;
            }
            
                        

            // prepare buffer for bufSize images and fill them with dummy values            
            size_t nx = 1024;
            size_t ny = 1024; 
            // array data to be written, 
            // contiguous memory block i required for buffered writing
            boost::shared_array<PixelType> arr(new PixelType[ nx * ny * bufSize]);
            for (size_t i = 0; i < (nx * ny * bufSize); ++i) {
                arr[i] = i + 5 + pid ;
            }

                // This hash will contain ArrayViews
                // Each ArrayView element will be mapped to a record in the Table
                Hash bufData;

                // first interpret the whole memory block as Array[bufSize,nx,ny]
                ArrayDimensions arrayDims(bufSize, nx, ny);
                ArrayView<PixelType> av(arr, arrayDims);

                // convert to ArrayView of arrays [nx,ny]
                ArrayView<ArrayView<PixelType> > imageBuffer = av.indexable();


                // now add data to the buffer
                bufData.set("array",imageBuffer);
            
                ArrayView<int> intBuffer(intArr,bufSize);            
                bufData.set("x", intBuffer);



            // this hash contains only one record, currently needed for format discovery
            // alternative way is to load format definition from xml file
            Hash dataDiscoveryHash;
            dataDiscoveryHash.set("x", intBuffer[0]);
            dataDiscoveryHash.set("array", imageBuffer[0]);

            DataFormat::Pointer dataFormat;
            Hash dfc;
            
            // set it to false if the format can be loaded from file
            bool discoverConfig = true;
            
            if (discoverConfig) {
                try {
                    dataFormat = DataFormat::discoverFromData(dataDiscoveryHash);
                } catch (...) {
                    RETHROW;
                }
                dfc = dataFormat->getConfig();
                tracer << "dataFormatConfig: " << endl << dfc << endl;
                Writer<Hash>::Pointer wc = Writer<Hash>::create(Hash("TextFile.filename", t.file("trainFormat.xml")));
                wc->write(dfc);
            } else {
                Reader<Hash>::Pointer rc = Reader<Hash>::create(Hash("TextFile.filename", t.file("trainFormat.xml")));
                rc->read(dfc);
            }
            dataFormat = DataFormat::create(dfc);


            tracer << "-----" << endl << dataDiscoveryHash << endl << "-----";

            // create table with defined chunkSize
            int chunkSize = 32;
            
	    long long t0 = exfel::util::Time::getMsSinceEpoch();
            
	    File file(t.file(filename));
            file.open(File::TRUNCATE);
            
	    Table::Pointer table = file.createTable("/test", dataFormat, chunkSize);
            tracer << "table created " << endl;

            long long t1 = exfel::util::Time::getMsSinceEpoch();
            // write buffer nBuffers time
            long long tStep = exfel::util::Time::getMsSinceEpoch();
            long long tStep1 = tStep;
            for (size_t i = 0; i < nBuffers; ++i) {

                // This hash will contain ArrayViews
                // Each ArrayView element will be mapped to a record in the Table
                Hash bufData;

                // first interpret the whole memory block as Array[bufSize,nx,ny]
                ArrayDimensions arrayDims(bufSize, nx, ny);
                ArrayView<PixelType> av(arr, arrayDims);

                // convert to ArrayView of arrays [nx,ny]
                ArrayView<ArrayView<PixelType> > imageBuffer = av.indexable();


                // now add data to the buffer
                bufData.set("array",imageBuffer);
            
                ArrayView<int> intBuffer(intArr,bufSize);            
                bufData.set("x", intBuffer);

                table->writeBuffer(bufData, i * intBuffer.getSize(), intBuffer.getSize());
		tStep1 = tStep;
                tStep = exfel::util::Time::getMsSinceEpoch();
                tracer << "time: " << (tStep - tStep1) << " [ms]" << endl;            
            }
            long long t2 = exfel::util::Time::getMsSinceEpoch();
            tracer << "Writing time: " << (t2 - t1) << " [ms]" << endl;            
	    file.close();
            long long t3 = exfel::util::Time::getMsSinceEpoch();
            tracer << "File open to close time: " << (t3 - t0) << " [ms]" << endl;            
                
        }
        return 0;
        

    } catch (Exception e) {
        cout << e;
        RETHROW
    }
    return 0;
}

