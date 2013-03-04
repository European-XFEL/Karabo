/* 
 * File:   H5File_Test.cc
 * Author: wrona
 * 
 * Created on February 20, 2013, 2:33 PM
 */

#include "H5File_Test.hh"
#include "karabo/util/ArrayTools.hh"
#include <karabo/util/Hash.hh>

#include <karabo/io/h5/File.hh>
#include <karabo/io/h5/Element.hh>
#include <karabo/io/h5/Scalar.hh>
#include <karabo/io/h5/ioProfiler.hh>

using namespace karabo::util;
using namespace karabo::io::h5;
using namespace karabo::io;

CPPUNIT_TEST_SUITE_REGISTRATION(H5File_Test);

H5File_Test::H5File_Test() {
}

H5File_Test::~H5File_Test() {
}

void H5File_Test::setUp() {
}

void H5File_Test::tearDown() {
}

void H5File_Test::testMethod() {

    {



        Hash data;

        data.set("instrument.a", 100);
        data.set("instrument.b", static_cast<float> (10));
        data.set("instrument.c", "abc");
        data.set("instrument.d", true).setAttribute("att1", 123);
        data.set("instrument.e", static_cast<char> (10));
        size_t s = 4 * 32 * 256;

        s = 1024 * 1024;
        vector<unsigned short> v0(s);
        for (size_t i = 0; i < v0.size(); ++i) {
            v0[i] = static_cast<unsigned short> (i % 16);
        }

        data.set("vectors.image", v0).setAttribute("dims", Dims(1024, 1024).toVector());



        s = 12;
        vector<double> v1(s);
        for (size_t i = 0; i < v1.size(); ++i) {
            v1[i] = i * 1.0 + 0.1234;
        }
        data.set("vectors.double", v1).setAttribute("dims", Dims(3, 4).toVector());

        s = 10;
        vector<bool> v2(s, false);
        for (size_t i = 0; i < v2.size(); ++i) {
            if (i % 2) v2[i] = true;
        }
        data.set("vectors.bool", v2).setAttribute("dims", Dims(2, 5).toVector());


        s = 5;
        ostringstream oss;
        oss << "vecString";
        vector<string> v3(s, "");
        for (size_t i = 0; i < v3.size(); ++i) {
            oss << " " << i;
            v3[i] = oss.str();
        }
        data.set("vectors.str", v3).setAttribute("dims", Dims(5).toVector());


        //data.setAttribute("instrument","at1",20);

        Hash config;
        Format::discoverFromHash(data, config);
        Format::Pointer dataFormat = Format::createFormat(config); 



//        clog << "config 1: " << dataFormat->getConfig() << endl;

        h5::Element::Pointer e1 = h5::Element::Pointer(new h5::Scalar<int>("experimental", "test", "experimental.test", 0));

        data.set("experimental.test", 198);
        dataFormat->addElement(e1);


        data.set("abecadlo.wer", 1006u);

        Hash uel(
                "h5path", "experimental",
                "h5name", "test23",
                "key", "abecadlo.wer",
                "type", "UINT32",
                "compressionLevel", 9
                );
        h5::Element::Pointer e2 = h5::Element::create("UINT32", uel);
        dataFormat->addElement(e2);

        dataFormat->removeElement("instrument/e");
        dataFormat->removeElement("instrument/d");
        dataFormat->removeElement("instrument/c");
        dataFormat->removeElement("instrument/b");
        dataFormat->removeElement("instrument/a");
        dataFormat->removeElement("vectors.image");
        dataFormat->removeElement("vectors.bool");
        dataFormat->removeElement("vectors.str");


//        clog << "config 2: " << dataFormat->getConfig() << endl;

        File file("file.h5");
        file.open(File::TRUNCATE);


        Table::Pointer t = file.createTable("/abc", dataFormat, 1);

        clog << "==================================" << endl;
        for (int i = 0; i < 3; ++i)
            t->write(data, i);


        file.close();


    }



}

void H5File_Test::testVectorBufferWrite() {


    return;
    clog << endl << "testVectorBufferWrite" << endl;
    Hash data;

    size_t nRec = 10;
    size_t s = 1024 * 1024 * nRec;
    vector<unsigned short> v0(s);
    for (size_t i = 0; i < v0.size(); ++i) {
        v0[i] = static_cast<unsigned short> (i % 10);
    }

    data.set("vectors.image", v0).setAttribute("dims", Dims(1024, 1024).toVector());

    File file("file2.h5");
    file.open(File::TRUNCATE);

    Hash config;
    Format::discoverFromHash(data, config);
    Format::Pointer dataFormat = Format::createNode("Format", "Format", config);

    Table::Pointer t = file.createTable("/abc", dataFormat, 1);

    int i = 0;
    //for (int i = 0; i < 5; ++i)
    t->write(data, i*nRec, nRec);


    clog << "abcd" << endl;

    file.close();


}

void H5File_Test::testBufferWrite() {

    {
        // clog << "TestBufferWrite" << endl;

        return;

        Hash data;

        data.set("instrument.a", 100);
        data.set("instrument.b", static_cast<float> (10));
        data.set("instrument.c", true);

        Hash config;
        Format::discoverFromHash(data, config);
        Format::Pointer dataFormat = Format::createNode("Format", "Format", config);


        clog << "after format discovery" << endl;
        const int bufSize = 100;
        vector<int> a(bufSize, 4);
        vector<float> b(bufSize, 5.2);
        bool c[bufSize];
        for (int i = 0; i < bufSize; ++i) {
            c[i] = false;
            if (i % 2) c[i] = true;
        }


        karabo::util::addPointerToHash(data, "instrument.a", &a[0], Dims(bufSize));
        karabo::util::addPointerToHash(data, "instrument.b", &b[0], Dims(bufSize));
        karabo::util::addPointerToHash(data, "instrument.c", &c[0], Dims(bufSize));



        //data.set("instrument.d", true);
        //data.set("instrument.e", static_cast<char> (10));

        //data.setAttribute("instrument","at1",20);





        File file("file1.h5");
        file.open(File::TRUNCATE);


        Table::Pointer t = file.createTable("/abc", dataFormat, 1000);

        for (int i = 0; i < 10; ++i)
            t->write(data, i * bufSize, bufSize);


        file.close();


    }



}
