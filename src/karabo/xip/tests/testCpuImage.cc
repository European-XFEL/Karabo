/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 31, 2011, 2:01 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <stdlib.h>
#include <iostream>

#include <exfel/util/Test.hh>

#include "../CpuImage.hh"
#include "../Memory.hh"

using namespace exfel::util;
using namespace exfel::xip;
using namespace std;


int testCpuImage(int argc, char** argv) {

    try {

        Test t;
        TEST_INIT(t, argc, argv);
        cout << t << endl;

        /***************************************
         *            Constructors             *
         ***************************************/

        {
            CpuImage<float> img;

            assert(img.dimensionality() == 0);
            assert(img.dimX() == 0);
            assert(img.dimY() == 0);
            assert(img.dimZ() == 0);
            assert(img.size() == 0);
            assert(img.byteSize() == 0);
            assert(img.pixelType() == "FLOAT");
        }

        {
            CpuImage<int> img(10);

            assert(img.dimensionality() == 1);
            assert(img.dimX() == 10);
            assert(img.dimY() == 1);
            assert(img.dimZ() == 1);
            assert(img.size() == 10);
            assert(img.byteSize() == 10 * sizeof (int));
            assert(img.pixelType() == "INT32");
        }

        {
            CpuImage<complex<float> > img(10, 5);

            assert(img.dimensionality() == 2);
            assert(img.dimX() == 10);
            assert(img.dimY() == 5);
            assert(img.dimZ() == 1);
            assert(img.size() == 50);
            assert(img.byteSize() == 50 * sizeof (complex<float>));
            assert(img.pixelType() == "COMPLEX_FLOAT");
        }

        {
            CpuImage<unsigned short > img(4, 4, 4);

            assert(img.dimensionality() == 3);
            assert(img.dimX() == 4);
            assert(img.dimY() == 4);
            assert(img.dimZ() == 4);
            assert(img.size() == 64);
            assert(img.byteSize() == 64 * sizeof (short));
            assert(img.pixelType() == "UINT16");
        }

        {
            CpuImage<float> img(4, 4, 1, 1.2);

            img.print();

            std::vector<char> buffer;
            exfel::io::BinarySerializer<CpuImage<float> >::Pointer serializer = exfel::io::BinarySerializer<CpuImage<float> >::create("Default");
            serializer->save(img, buffer);
            std::cout << buffer.size() << std::endl;

            CpuImage<float> img2;
            serializer->load(img2, &buffer[0], buffer.size());
            img2.print();
        }

        {
            CpuImage<float> img(4, 4, 1, 1.2);

            unsigned int channelId = Memory<CpuImage<float> >::registerChannel("a");
            unsigned int chunkId = Memory<CpuImage<float> >::registerChunk(channelId);
            Memory<CpuImage<float> >::write(img, channelId, chunkId);
            img.setHeader(Hash("New entry", "Indeed"));
            Memory<CpuImage<float> >::write(img, channelId, chunkId);

            CpuImage<float> tgt;
            Memory<CpuImage<float> >::read(tgt, 0, channelId, chunkId);
            tgt.print("FromCache");

            vector<char> buffer;
            exfel::util::Hash header;
            Memory<CpuImage<float> >::readAsContiguosBlock(buffer, header, channelId, chunkId);
            cout << header;

            unsigned int channelId2 = Memory<CpuImage<float> >::registerChannel("b");
            unsigned int chunkId2 = Memory<CpuImage<float> >::registerChunk(channelId);
            Memory<CpuImage<float> >::writeAsContiguosBlock(buffer, header, channelId2, chunkId2);

            CpuImage<float> result;
            Memory<CpuImage<float> >::read(result, 1, channelId2, chunkId2);
            result.print();

        }
        {
            unsigned int channelId = Memory<Hash>::registerChannel("bla");
            unsigned int chunkId = Memory<Hash>::registerChunk(channelId);
            Hash h("This.is.a", "test");
            for (size_t i = 0; i < 100; ++i) {
                Memory<Hash>::write(h, channelId, chunkId);
            }
            vector<char> buffer;
            Hash header;
            Memory<Hash>::readAsContiguosBlock(buffer, header, channelId, chunkId);
            cout << header;


        }






    } catch (...) {
        RETHROW
    }


    return (EXIT_SUCCESS);
}

