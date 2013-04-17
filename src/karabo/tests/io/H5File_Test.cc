/* 
 * File:   H5File_Test.cc
 * Author: wrona
 * 
 * Created on February 20, 2013, 2:33 PM
 */

#include "H5File_Test.hh"
#include "karabo/util/ArrayTools.hh"
#include "TestPathSetup.hh"
#include <karabo/util/Hash.hh>

#include <karabo/io/h5/File.hh>
#include <karabo/io/h5/Element.hh>
#include <karabo/io/h5/Scalar.hh>
#include <karabo/io/h5/ioProfiler.hh>

#include <karabo/log/Tracer.hh>

using namespace karabo::util;
using namespace karabo::io::h5;
using namespace karabo::io;
using namespace log4cpp;

CPPUNIT_TEST_SUITE_REGISTRATION(H5File_Test);


H5File_Test::H5File_Test() {

    karabo::log::Tracer tr;
    tr.disableAll();

    //    tr.enable("karabo.io.h5.Table");
    //    tr.enable("karabo.io.h5.Table.saveTableFormatAsAttribute");
    //    tr.enable("karabo.io.h5.Table.openNew");
    //    tr.enable("karabo.io.h5.Table.openReadOnly");
    //    tr.enable("H5File_Test.testReadTable");
    tr.reconfigure();


    m_numberOfRecords = 512;
}


H5File_Test::~H5File_Test() {
}


void H5File_Test::setUp() {

    m_v3Size = 5;
    ostringstream oss;
    oss << "vecString";
    m_v3 = vector<string>(m_v3Size, "");
    for (size_t i = 0; i < m_v3.size(); ++i) {
        oss << " " << i;
        m_v3[i] = oss.str();
    }

}


void H5File_Test::tearDown() {
}


void H5File_Test::testWrite() {



    try {

        Hash data;

        data.set("instrument.a", 100);
        data.set("instrument.b", static_cast<float> (10));
        data.set("instrument.c", "abc");
        data.set("instrument.d", true).setAttribute("att1", 123);
        data.set("instrument.e", static_cast<char> (58));
        
        size_t s = 4 * 32 * 256;
        //s = 1024 * 1024;
        vector<unsigned short> v0(s);
        for (size_t i = 0; i < v0.size(); ++i) {
            v0[i] = static_cast<unsigned short> (i % 16);
        }

        data.set("instrument.LPD.image", &v0[0]).setAttribute("dims", Dims(4,32,256).toVector());



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




        Hash i32el(
                   "h5path", "experimental",
                   "h5name", "test",
                   "key", "experimental.test",
                   "compressionLevel", 9
                   );

        h5::Element::Pointer e1 = h5::Element::create("INT32", i32el);

        data.set("experimental.test", 198);
        dataFormat->addElement(e1);


        data.set("abecadlo.wer", 1006u);

        Hash uel(
                 "h5path", "experimental",
                 "h5name", "test23",
                 "key", "abecadlo.wer",
                 "compressionLevel", 9
                 );
        h5::Element::Pointer e2 = h5::Element::create("UINT32", uel);
        dataFormat->addElement(e2);

        //        dataFormat->removeElement("instrument/e");
        //        dataFormat->removeElement("instrument/d");
        //        dataFormat->removeElement("instrument/c");
        //        dataFormat->removeElement("instrument/b");
        //        dataFormat->removeElement("instrument/a");
        //        dataFormat->removeElement("vectors.image");
        //        dataFormat->removeElement("vectors.bool");
        //        dataFormat->removeElement("vectors.str");


        //        clog << "config 2: " << dataFormat->getConfig() << endl;

        File file(resourcePath("file.h5"));
        file.open(File::TRUNCATE);


        Table::Pointer t = file.createTable("/abc", dataFormat, 1);


        for (size_t i = 0; i < m_numberOfRecords; ++i)
            t->write(data, i);


        file.close();



    } catch (karabo::util::Exception& e) {
        cerr << e.detailedMsg() << endl;
        KARABO_RETHROW;
    }



}


void H5File_Test::testRead() {

    try {

        //Define what you want to read from file.h5 which has been written by testWrite
        // we read:
        //    /abc/experimental/test23 into Hash element data("bla")  - UINT32
        //    /abc/vectors/double  into Hash element data("db")       - VECTOR_DOUBLE
        //    /abc/instrument/d    into Hash element data("d")        - BOOL
        //    /abc/vectors/bool    into Hash element data(("a.bools") - PTR_BOOL
        //    /abc/instrument/c    into Hash element data("c")        - STRING

        Format::Pointer format = Format::createEmptyFormat();
        Hash c1(
                "h5path", "experimental",
                "h5name", "test23",
                "key", "bla"
                );

        h5::Element::Pointer e1 = h5::Element::create("UINT32", c1);
        format->addElement(e1);

        Dims dims(3, 4);
        Hash c2;
        c2.set("h5path", "vectors");
        c2.set("h5name", "double");
        c2.set("key", "db");
        c2.set("dims", dims.toVector());


        h5::Element::Pointer e2 = h5::Element::create("VECTOR_DOUBLE", c2);
        format->addElement(e2);


        Hash c3(
                "h5path", "instrument",
                "h5name", "d",
                "key", "d"
                );

        h5::Element::Pointer e3 = h5::Element::create("BOOL", c3);
        format->addElement(e3);

        Dims boolDims(2, 5);
        Hash c4(
                "h5path", "vectors",
                "h5name", "bool",
                "key", "a.bools",
                "dims", boolDims.toVector()
                );

        h5::Element::Pointer e4 = h5::Element::create("VECTOR_BOOL", c4);
        format->addElement(e4);


        Hash c5(
                "h5path", "instrument",
                "h5name", "c",
                "key", "c"
                );

        h5::Element::Pointer e5 = h5::Element::create("STRING", c5);
        format->addElement(e5);


        Dims stringsDims(5);
        Hash c6(
                "h5path", "vectors",
                "h5name", "str",
                "key", "strings",
                "dims", stringsDims.toVector()
                );

        h5::Element::Pointer e6 = h5::Element::create("VECTOR_STRING", c6);
        format->addElement(e6);


        // End of definition of what is read



        // Open the file in READONLY mode
        File file(resourcePath("file.h5"));
        file.open(File::READONLY);


        // Define the table to be read using defined format
        Table::Pointer table = file.getTable("/abc", format);

        // Declare container for data
        Hash data;

        // Bind some variable to hash data
        // Note that hash element "d" is not bound

        // bla will contain data from /abc/experimental/test23
        unsigned int& bla = data.bindReference<unsigned int>("bla");

        // vecd will contain data from /abc/vectors/double
        // here we are responsible for memory management
        vector<double>& vecd = data.bindReference<vector<double> >("db");
        vecd.resize(dims.size());

        // bArray will contain data from /abc/vectors/bool
        // here we are responsible for memory management
        bool bArray[boolDims.size()];
        data.set("a.bools", &bArray[0]);

        vector<string>& strings = data.bindReference<vector<string> >("strings");
        strings.resize(stringsDims.size());

        // /abc/instrument/d will be accessible as Hash element with key "d"
        // because we do not use binding
        // Memory is managed by our API


        // Now bind the data Hash to table
        table->bind(data);

        // get number of records in the file
        size_t nRecords = table->getNumberOfRecords();

        KARABO_LOG_FRAMEWORK_TRACE << "number of records: " << nRecords;
        CPPUNIT_ASSERT(nRecords == m_numberOfRecords);



        // read first record
        table->read(0);

        KARABO_LOG_FRAMEWORK_TRACE << "after reading: ";

        // assert values
        CPPUNIT_ASSERT(bla == 1006);
        CPPUNIT_ASSERT(data.get<unsigned int>("bla") == 1006);
        vector<double>& vec = data.get< vector<double> >("db");
        for (size_t i = 0; i < dims.size(); ++i) {
            CPPUNIT_ASSERT(vec[i] == i * 1.0 + 0.1234);
            CPPUNIT_ASSERT(vecd[i] == i * 1.0 + 0.1234);
        }

        CPPUNIT_ASSERT(data.get<bool>("d") == true);

        for (size_t i = 0; i < boolDims.size(); ++i) {
            if (i % 2) CPPUNIT_ASSERT(bArray[i] == true);
            else CPPUNIT_ASSERT(bArray[i] == false);

        }
        //        clog << endl;

        KARABO_LOG_FRAMEWORK_TRACE << "string reference value: abc, actual value: " << data.get<string>("c");
        CPPUNIT_ASSERT(data.get<string>("c") == "abc");

        //vector<string>& strings = data.get<vector<string> >("strings");

        for (size_t i = 0; i < stringsDims.size(); ++i) {
            KARABO_LOG_FRAMEWORK_TRACE << "strings[" << i << "] = " << strings[i];
            CPPUNIT_ASSERT(strings[i] == m_v3[i]);
        }



        //read second record
        table->read(1);


    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_ASSERT(true == false);
    }

}


void H5File_Test::testReadTable() {

    try {
        // Open the file in READONLY mode
        File file(resourcePath("file.h5"));
        file.open(File::READONLY);


        // Define the table to be read using defined format
        Table::Pointer table = file.getTable("/abc");

        // Declare container for data
        Hash data;

        // test23 will contain data from /abc/experimental/test23
        unsigned int& test23 = data.bindReference<unsigned int>("experimental.test23");

        table->bind(data);

        table->read(0);

        KARABO_LOG_FRAMEWORK_TRACE_CF << "DATA:\n" << data;

        KARABO_LOG_FRAMEWORK_TRACE_CF << "test23: " << test23;
        CPPUNIT_ASSERT(test23 == 1006);

        float b = data.get<float>("instrument.b");
        KARABO_LOG_FRAMEWORK_TRACE_CF << "data(\"instrument.b\"): " << b;
        CPPUNIT_ASSERT(b > 9.99999);
        CPPUNIT_ASSERT(b < 10.00001);
        CPPUNIT_ASSERT(data.get<int>("instrument.a") == 100);
        CPPUNIT_ASSERT(data.get<string>("instrument.c") == "abc");
        CPPUNIT_ASSERT(data.get<bool>("instrument.d") == true);

        //TODO  CPPUNIT_ASSERT( data.getAttribute<int>("instrument.d","att1") == 123 );
        CPPUNIT_ASSERT(data.get<char>("instrument.e") == 58);

        file.close();
    } catch (Exception& ex) {
        clog << ex << endl;
        CPPUNIT_ASSERT(true == false);
    }


}


void H5File_Test::testVectorBufferWrite() {


    return;
    //    clog << endl << "testVectorBufferWrite" << endl;
    Hash data;

    size_t nRec = 10;
    size_t s = 1024 * 1024 * nRec;
    vector<unsigned short> v0(s);
    for (size_t i = 0; i < v0.size(); ++i) {
        v0[i] = static_cast<unsigned short> (i % 10);
    }

    data.set("vectors.image", v0).setAttribute("dims", Dims(1024, 1024).toVector());

    File file(resourcePath("file2.h5"));
    file.open(File::TRUNCATE);

    Hash config;
    Format::discoverFromHash(data, config);
    Format::Pointer dataFormat = Format::createNode("Format", "Format", config);

    Table::Pointer t = file.createTable("/abc", dataFormat, 1);

    int i = 0;
    //for (int i = 0; i < 5; ++i)
    t->write(data, i*nRec, nRec);


    //    clog << "abcd" << endl;

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


        //        clog << "after format discovery" << endl;
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





        File file(resourcePath("file1.h5"));
        file.open(File::TRUNCATE);


        Table::Pointer t = file.createTable("/abc", dataFormat, 1000);

        for (int i = 0; i < 10; ++i)
            t->write(data, i * bufSize, bufSize);


        file.close();


    }



}
